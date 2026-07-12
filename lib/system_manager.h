#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

typedef enum
{
    SYSTEM_MANAGER_SUCCESS = 0,
    SYSTEM_MANAGER_INITIALIZATION_ERROR,
    SYSTEM_MANAGER_DRIVER_ERROR,
    SYSTEM_MANAGER_INVALID_HARDWARE_REVISION,
    SYSTEM_MANAGER_TEMPERATURE_CONVERSION_ERROR
} SystemManagerStatus;

SystemManagerStatus SystemManager_Init(void);
SystemManagerStatus SystemManager_Process(void);
void SystemManager_Run(void);

#endif /* SYSTEM_MANAGER_H */
