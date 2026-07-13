#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "system_types.h"
#include "xgpio.h"
#include "xil_types.h"

typedef enum
{
    GPIO_DRIVER_SUCCESS = 0,
    GPIO_DRIVER_INVALID_ARGUMENT,
    GPIO_DRIVER_INITIALIZATION_ERROR,
    GPIO_DRIVER_INVALID_STATE
} GpioDriverStatus;

typedef struct
{
    XGpio instance;
    u32 channel;
} GpioDriver;

GpioDriverStatus GpioDriver_Initialize(
    GpioDriver *driver,
    u16 device_id,
    u32 channel);

GpioDriverStatus GpioDriver_ShowTemperatureState(
    GpioDriver *driver,
    TemperatureState state);

void GpioDriver_ShowError(
    GpioDriver *driver);

void GpioDriver_AllOff(
    GpioDriver *driver);

#endif
