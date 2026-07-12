#ifndef TEMPERATURE_DRIVER_H
#define TEMPERATURE_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    TEMPERATURE_DRIVER_SUCCESS = 0,
    TEMPERATURE_DRIVER_NOT_INITIALIZED,
    TEMPERATURE_DRIVER_INVALID_ARGUMENT,
    TEMPERATURE_DRIVER_DATA_NOT_READY,
    TEMPERATURE_DRIVER_SELF_TEST_FAILED
} TemperatureDriverStatus;

typedef struct
{
    uint32_t raw_value;
    uint32_t status_register;
} TemperatureSample;

TemperatureDriverStatus TemperatureDriver_Init(uintptr_t base_address);
bool TemperatureDriver_IsInitialized(void);
TemperatureDriverStatus TemperatureDriver_GetDataReady(bool *is_ready);
TemperatureDriverStatus TemperatureDriver_Read(TemperatureSample *sample);
TemperatureDriverStatus TemperatureDriver_AcknowledgeInterrupt(void);
TemperatureDriverStatus TemperatureDriver_SelfTest(void);

#endif /* TEMPERATURE_DRIVER_H */
