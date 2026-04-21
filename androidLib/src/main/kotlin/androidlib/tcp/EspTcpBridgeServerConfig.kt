package androidlib.tcp

data class EspTcpBridgeServerConfig(
    val bindHost: String = "0.0.0.0",
    val port: Int = 8888,
    val readBufferSize: Int = 8192,
)
