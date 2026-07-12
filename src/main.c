#include "system_manager.h"

#include "xil_printf.h"

int main(void)
{
    SystemManagerStatus status;

    xil_printf("\r\n");
    xil_printf("ZCU104 temperature monitor starting\r\n");

    status = SystemManager_Init();

    if (status != SYSTEM_MANAGER_SUCCESS)
    {
        xil_printf(
            "Fatal system initialization error: %d\r\n",
            (int)status);

        return -1;
    }

    SystemManager_Run();

    return 0;
}
