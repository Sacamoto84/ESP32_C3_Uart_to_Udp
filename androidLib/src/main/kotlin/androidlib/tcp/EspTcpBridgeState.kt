package androidlib.tcp

sealed interface EspTcpBridgeState {
    data object Stopped : EspTcpBridgeState

    data class Listening(
        val host: String,
        val port: Int,
    ) : EspTcpBridgeState

    data class ClientConnected(
        val remoteAddress: String,
    ) : EspTcpBridgeState

    data class ClientDisconnected(
        val remoteAddress: String,
    ) : EspTcpBridgeState

    data class Error(
        val throwable: Throwable,
    ) : EspTcpBridgeState
}
