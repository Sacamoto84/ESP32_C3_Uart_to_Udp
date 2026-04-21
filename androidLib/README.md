# Android TCP Helper

This folder contains a small Android-side TCP helper for the ESP32 bridge.

The current ESP32 firmware in TCP mode acts as a **TCP client** and connects to
the Android device at `ipClient:8888`.

That means the Android app should act as a **TCP server**.

## What is inside

- `EspTcpBridgeServerConfig.kt` - server config
- `EspTcpBridgeState.kt` - connection state model
- `EspTcpBridgeServer.kt` - Ktor-based TCP server for Android

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

The helper accepts one active ESP32 connection at a time and exposes received
bytes as `Flow<ByteArray>`.

## Example usage

```kotlin
class BridgeViewModel : ViewModel() {

    private val tcpServer = EspTcpBridgeServer(
        scope = viewModelScope,
        config = EspTcpBridgeServerConfig(
            bindHost = "0.0.0.0",
            port = 8888,
            readBufferSize = 8192,
        )
    )

    val state = tcpServer.state

    init {
        tcpServer.start()

        viewModelScope.launch {
            tcpServer.incomingChunks.collect { chunk ->
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
            tcpServer.stop()
        }
        super.onCleared()
    }
}
```

## How to use with ESP32

1. Start this TCP server in the Android app.
2. Find the Android device IP on the local Wi-Fi network.
3. Put that IP into the ESP32 settings portal as `ipClient`.
4. Enable `TCP transport` in the ESP32 settings portal.
5. Keep port `8888` on the Android side.

## Optional send back to ESP32

The helper also contains:

```kotlin
suspend fun send(bytes: ByteArray)
suspend fun sendText(text: String)
```

You can use them if the Android app needs to send commands back to ESP32 over
the same TCP connection.
