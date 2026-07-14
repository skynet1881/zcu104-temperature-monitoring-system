#include "system_manager.hpp"
#include "xil_printf.h"

int main()
{
    xil_printf("\r\n");
    xil_printf("--------------------------------------------\r\n");
    xil_printf("ZCU104 Temperature Monitoring System \n");
    xil_printf("--------------------------------------------\r\n");

    app::SystemManager system_manager;
    app::SystemManager::Status status = system_manager.initialize();

    if (status != app::SystemManager::Status::Success)
    {
        xil_printf(
            "System initialization failed: %d\r\n",
            static_cast<int>(status));
        return static_cast<int>(status);
    }

    for (;;)
    {
        status = system_manager.process();

        if (status != app::SystemManager::Status::Success)
        {
            xil_printf(
                "System processing error: %d\r\n",
                static_cast<int>(status));
        }
    }

    return 0;
}
