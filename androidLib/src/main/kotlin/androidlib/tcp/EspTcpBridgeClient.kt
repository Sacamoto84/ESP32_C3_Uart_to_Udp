package androidlib.tcp

import io.ktor.network.selector.SelectorManager
import io.ktor.network.sockets.Socket
import io.ktor.network.sockets.aSocket
import io.ktor.utils.io.ByteWriteChannel
import io.ktor.utils.io.readAvailable
import io.ktor.utils.io.writeFully
import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext

/**
 * Android-side TCP client for the ESP32 bridge.
 *
 * ESP32 in the current firmware listens as a TCP server on port 8888, and the
 * Android app connects to it as a single TCP client.
 */
class EspTcpBridgeClient(
    scope: CoroutineScope,
    private val config: EspTcpBridgeClientConfig,
) {
    private val clientScope = CoroutineScope(scope.coroutineContext + SupervisorJob() + Dispatchers.IO)
    private val writerMutex = Mutex()
    private val started = AtomicBoolean(false)

    private var selectorManager: SelectorManager? = null
    private var clientSocket: Socket? = null
    private var outputChannel: ByteWriteChannel? = null
    private var clientJob: Job? = null

    private val _state = MutableStateFlow<EspTcpBridgeState>(EspTcpBridgeState.Stopped)
    val state: StateFlow<EspTcpBridgeState> = _state.asStateFlow()

    // TCP keeps byte order, but not message boundaries.
    private val incomingChannel = Channel<ByteArray>(capacity = Channel.BUFFERED)
    val incomingChunks: Flow<ByteArray> = incomingChannel.receiveAsFlow()

    fun start() {
        if (!started.compareAndSet(false, true)) {
            return
        }

        clientJob = clientScope.launch {
            runClientLoop()
        }
    }

    suspend fun stop() {
        started.set(false)

        closeClient()

        clientJob?.cancelAndJoin()
        clientJob = null

        selectorManager?.close()
        selectorManager = null

        _state.value = EspTcpBridgeState.Stopped
    }

    suspend fun send(bytes: ByteArray) {
        writerMutex.withLock {
            val channel = outputChannel ?: error("No ESP32 connection")
            channel.writeFully(bytes)
        }
    }

    suspend fun sendText(text: String) {
        send(text.toByteArray(Charsets.UTF_8))
    }

    private suspend fun runClientLoop() {
        selectorManager = SelectorManager(Dispatchers.IO)
        val manager = selectorManager ?: return

        try {
            while (started.get() && currentCoroutineContext().isActive) {
                _state.value = EspTcpBridgeState.Connecting(config.host, config.port)

                try {
                    val socket = aSocket(manager).tcp().connect(config.host, config.port)
                    handleConnectedSocket(socket)
                } catch (cancelled: CancellationException) {
                    throw cancelled
                } catch (t: Throwable) {
                    if (started.get()) {
                        _state.value = EspTcpBridgeState.Error(t)
                    }
                }

                if (started.get() && currentCoroutineContext().isActive) {
                    _state.value = EspTcpBridgeState.Disconnected
                    delay(config.reconnectDelayMs)
                }
            }
        } finally {
            closeClient()
        }
    }

    private suspend fun handleConnectedSocket(socket: Socket) {
        val remoteAddress = remoteAddressOf(socket)
        val input = socket.openReadChannel()
        val output = socket.openWriteChannel(autoFlush = true)
        val buffer = ByteArray(config.readBufferSize)

        writerMutex.withLock {
            clientSocket = socket
            outputChannel = output
        }
        _state.value = EspTcpBridgeState.Connected(remoteAddress)

        try {
            while (started.get() && currentCoroutineContext().isActive) {
                val read = input.readAvailable(buffer, 0, buffer.size)
                if (read == -1) {
                    break
                }

                if (read == 0) {
                    continue
                }

                incomingChannel.send(buffer.copyOf(read))
            }
        } catch (cancelled: CancellationException) {
            throw cancelled
        } catch (t: Throwable) {
            if (started.get()) {
                _state.value = EspTcpBridgeState.Error(t)
            }
        } finally {
            closeClient()
        }
    }

    private suspend fun closeClient() {
        writerMutex.withLock {
            runCatching { outputChannel?.close() }
            runCatching { clientSocket?.close() }
            outputChannel = null
            clientSocket = null
        }
    }

    private fun remoteAddressOf(socket: Socket): String {
        return runCatching { socket.remoteAddress.toString() }
            .getOrElse { "unknown" }
    }
}
