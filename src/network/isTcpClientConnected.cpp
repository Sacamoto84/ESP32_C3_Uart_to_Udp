#include "network_internal.h"

// Отдаёт кэшированное состояние потокового TCP-клиента для интерфейса и диагностики.
bool isTcpClientConnected()
{
    return tcpClientConnected;
}
