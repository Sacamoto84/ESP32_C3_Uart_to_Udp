#include "network_internal.h"

// Широковещательная отправка на 255.255.255.255:8888.
bool sendUdpBroadcast(const char *msg, int len)
{
    sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(kNetworkDataPort);
    dest.sin_addr.s_addr = inet_addr("255.255.255.255");

    return sendUdpPacket("sendUdpBroadcast", msg, len, dest, true);
}
