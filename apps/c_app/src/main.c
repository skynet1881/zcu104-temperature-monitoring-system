#include "system_manager.h"
#include "xil_printf.h"

int main(void)
{
    SystemManagerStatus status;

    xil_printf("\r\n");
    xil_printf("ZCU104 Temperature Monitoring System\r\n");
    xil_printf("------------------------------------\r\n");

    // Init the system manager, which initializes the drivers and reads the configuration from EEPROM.
    status = SystemManager_Initialize();

    if (status != SYSTEM_MANAGER_SUCCESS)
    {
        xil_printf(
            "System initialization failed: %d\r\n",
            (int)status);

        return (int)status;
    }

    for (;;)
    {
        status = SystemManager_Process();

        if (status != SYSTEM_MANAGER_SUCCESS)
        {
            /*
             * Do not print from the ISR. Runtime errors are reported here
             * from normal execution context.
             */
            xil_printf(
                "System processing error: %d\r\n",
                (int)status);
        }
    }

    return 0;
}
