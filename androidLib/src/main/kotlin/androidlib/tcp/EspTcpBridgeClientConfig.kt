package androidlib.tcp

data class EspTcpBridgeClientConfig(
    val host: String,
    val port: Int = 8888,
    val readBufferSize: Int = 8192,
    val reconnectDelayMs: Long = 1000,
)
