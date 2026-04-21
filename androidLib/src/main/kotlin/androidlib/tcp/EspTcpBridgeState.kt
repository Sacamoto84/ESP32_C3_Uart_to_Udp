package androidlib.tcp

sealed interface EspTcpBridgeState {
    data object Stopped : EspTcpBridgeState

    data class Connecting(
        val host: String,
        val port: Int,
    ) : EspTcpBridgeState

    data class Connected(
        val remoteAddress: String,
    ) : EspTcpBridgeState

    data object Disconnected : EspTcpBridgeState

    data class Error(
        val throwable: Throwable,
    ) : EspTcpBridgeState
}
