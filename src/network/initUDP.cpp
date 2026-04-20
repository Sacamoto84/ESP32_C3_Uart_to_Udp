#include "network_internal.h"

// Инициализация UDP для внешнего экрана.
// В безэкранной сборке порт 82 вообще не поднимаем, потому что принимать framebuffer некому.
void initUDP()
{
#if !PROJECT_HAS_SCREEN
    return;
#else
    udp.begin(82);
#endif
}
