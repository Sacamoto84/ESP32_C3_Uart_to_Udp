package androidlib.tcp

import io.ktor.network.selector.SelectorManager
import io.ktor.network.sockets.ServerSocket
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
 * Android-side TCP server for the ESP32 bridge.
 *
 * ESP32 in this firmware connects as a TCP client, so Android should listen on
 * the configured port and accept exactly one active device connection.
 */
class EspTcpBridgeServer(
    scope: CoroutineScope,
    private val config: EspTcpBridgeServerConfig = EspTcpBridgeServerConfig(),
) {
    private val serverScope = CoroutineScope(scope.coroutineContext + SupervisorJob() + Dispatchers.IO)
    private val writerMutex = Mutex()
    private val started = AtomicBoolean(false)

    private var selectorManager: SelectorManager? = null
    private var serverSocket: ServerSocket? = null
    private var clientSocket: Socket? = null
    private var outputChannel: ByteWriteChannel? = null
    private var serverJob: Job? = null

    private val _state = MutableStateFlow<EspTcpBridgeState>(EspTcpBridgeState.Stopped)
    val state: StateFlow<EspTcpBridgeState> = _state.asStateFlow()

    // Raw byte chunks received from TCP. Ordering is preserved, but chunk
    // boundaries should not be treated as message boundaries.
    private val incomingChannel = Channel<ByteArray>(capacity = Channel.BUFFERED)
    val incomingChunks: Flow<ByteArray> = incomingChannel.receiveAsFlow()

    fun start() {
        if (!started.compareAndSet(false, true)) {
            return
        }

        serverJob = serverScope.launch {
            runServerLoop()
        }
    }

    suspend fun stop() {
        started.set(false)

        // Closing sockets first helps unblock accept()/readAvailable() so the
        // coroutine can finish cleanly without hanging on shutdown.
        closeServer()
        closeClient()

        serverJob?.cancelAndJoin()
        serverJob = null

        selectorManager?.close()
        selectorManager = null

        _state.value = EspTcpBridgeState.Stopped
    }

    suspend fun send(bytes: ByteArray) {
        writerMutex.withLock {
            val channel = outputChannel ?: error("No ESP32 client connected")
            channel.writeFully(bytes)
        }
    }

    suspend fun sendText(text: String) {
        send(text.toByteArray(Charsets.UTF_8))
    }

    private suspend fun runServerLoop() {
        selectorManager = SelectorManager(Dispatchers.IO)
        val manager = selectorManager ?: return

        try {
            serverSocket = aSocket(manager).tcp().bind(config.bindHost, config.port)
            _state.value = EspTcpBridgeState.Listening(config.bindHost, config.port)

            while (started.get() && currentCoroutineContext().isActive) {
                val socket = serverSocket?.accept() ?: break
                handleClient(socket)

                if (started.get() && currentCoroutineContext().isActive) {
                    _state.value = EspTcpBridgeState.Listening(config.bindHost, config.port)
                }
            }
        } catch (cancelled: CancellationException) {
            throw cancelled
        } catch (t: Throwable) {
            if (started.get()) {
                _state.value = EspTcpBridgeState.Error(t)
            }
        } finally {
            closeClient()
            closeServer()
        }
    }

    private suspend fun handleClient(socket: Socket) {
        val remoteAddress = remoteAddressOf(socket)
        val input = socket.openReadChannel()
        val output = socket.openWriteChannel(autoFlush = true)
        val buffer = ByteArray(config.readBufferSize)

        writerMutex.withLock {
            clientSocket = socket
            outputChannel = output
        }
        _state.value = EspTcpBridgeState.ClientConnected(remoteAddress)

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
            if (started.get()) {
                _state.value = EspTcpBridgeState.ClientDisconnected(remoteAddress)
            }
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

    private suspend fun closeServer() {
        withContext(Dispatchers.IO) {
            runCatching { serverSocket?.close() }
            serverSocket = null
        }
    }

    private fun remoteAddressOf(socket: Socket): String {
        return runCatching { socket.remoteAddress.toString() }
            .getOrElse { "unknown" }
    }
}
