#include "network_internal.h"

// Инициализация UDP:
// - heartbeat/ping-pong всегда слушаем на 8888
// - UDP внешнего экрана на 82 поднимаем только когда экран действительно есть
void initUDP()
{
    heartbeatUdp.begin(kHeartbeatPort);

#if PROJECT_HAS_SCREEN
    udp.begin(82);
#endif
}
