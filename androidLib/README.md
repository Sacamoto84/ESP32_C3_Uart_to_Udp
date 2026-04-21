# Android TCP Helper

This folder contains a small Android-side TCP helper for the ESP32 bridge.

The current ESP32 firmware listens as a **TCP server** on port `8888`.

That means the Android app should act as a **TCP client** and connect to the
ESP32 IP address.

## What is inside

- `EspTcpBridgeClientConfig.kt` - client config
- `EspTcpBridgeState.kt` - connection state model
- `EspTcpBridgeClient.kt` - Ktor-based TCP client for Android

## Dependencies

Ktor official sockets docs:

- https://ktor.io/docs/server-sockets.html
- https://api.ktor.io/ktor-network/io.ktor.network.sockets/index.html

Gradle dependencies:

```kotlin
dependencies {
    implementation("io.ktor:ktor-network:$ktorVersion")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:$coroutinesVersion")
}
```

You can keep `ktorVersion` and `coroutinesVersion` in your version catalog or in
top-level Gradle properties.

Android manifest:

```xml
<uses-permission android:name="android.permission.INTERNET" />
```

## Important note

TCP preserves **byte order**, but it does **not** preserve original write
boundaries.

If ESP32 writes:

```text
ABC
DEF
```

Android may receive it as:

```text
ABCDEF
```

or:

```text
A
BCDE
F
```

So you should treat `incomingChunks` as a **byte stream**, not as packets.

The helper keeps reconnecting while started. If the ESP32 is temporarily
unavailable, Android will try to reconnect after `reconnectDelayMs`.

## Example usage

```kotlin
class BridgeViewModel : ViewModel() {

    private val tcpClient = EspTcpBridgeClient(
        scope = viewModelScope,
        config = EspTcpBridgeClientConfig(
            host = "192.168.4.1",
            port = 8888,
            readBufferSize = 8192,
            reconnectDelayMs = 1000,
        )
    )

    val state = tcpClient.state

    init {
        tcpClient.start()

        viewModelScope.launch {
            tcpClient.incomingChunks.collect { chunk ->
                // Feed your UART protocol parser here.
                // Order is preserved by TCP.
                handleBytes(chunk)
            }
        }
    }

    private fun handleBytes(bytes: ByteArray) {
        // TODO: parse your stream here
    }

    override fun onCleared() {
        viewModelScope.launch {
            tcpClient.stop()
        }
        super.onCleared()
    }
}
```

## How to use with ESP32

1. Connect Android to the same Wi-Fi network as ESP32, or connect to the ESP32 access point.
2. Find the ESP32 IP address in the serial log, on the OLED, or in the settings portal.
3. Start `EspTcpBridgeClient` with `host = "<ESP32 IP>"` and `port = 8888`.
4. Keep `broadcast` disabled if you want normal TCP transport.
5. If `broadcast` is enabled in the ESP32 portal, data will go over UDP broadcast instead of TCP.

## Optional send back to ESP32

The helper also contains:

```kotlin
suspend fun send(bytes: ByteArray)
suspend fun sendText(text: String)
```

You can use them if the Android app needs to send commands back to ESP32 over
the same TCP connection.
