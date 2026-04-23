#include "network_internal.h"

// Поднимает UDP-порты для heartbeat и, при наличии экрана, для внешнего потока кадров.
void initUDP()
{
    heartbeatUdp.begin(kHeartbeatPort);

#if PROJECT_HAS_SCREEN
    udp.begin(82);
#endif
}
