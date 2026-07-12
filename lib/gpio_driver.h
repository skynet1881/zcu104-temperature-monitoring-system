#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    GPIO_DRIVER_SUCCESS = 0,
    GPIO_DRIVER_NOT_INITIALIZED,
    GPIO_DRIVER_INVALID_ARGUMENT,
    GPIO_DRIVER_INITIALIZATION_ERROR
} GpioDriverStatus;

typedef struct
{
    uint16_t device_id;
    unsigned int channel;
    uint32_t output_mask;
    uint32_t green_led_mask;
    uint32_t yellow_led_mask;
    uint32_t red_led_mask;
} GpioDriverConfig;

GpioDriverStatus GpioDriver_Init(
    const GpioDriverConfig *config);

bool GpioDriver_IsInitialized(void);

GpioDriverStatus GpioDriver_SetLedPattern(
    uint32_t pattern);

GpioDriverStatus GpioDriver_ShowTemperature(
    int32_t temperature_x10);

GpioDriverStatus GpioDriver_ShowError(void);
GpioDriverStatus GpioDriver_AllOff(void);

#endif /* GPIO_DRIVER_H */
