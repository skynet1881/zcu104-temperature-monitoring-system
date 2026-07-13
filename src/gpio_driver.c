#include "gpio_driver.h"
#include <stddef.h>

#include "system_config.h"
#include "xstatus.h"

GpioDriverStatus GpioDriver_Initialize(
    GpioDriver *driver,
    u16 device_id,
    u32 channel)
{
    s32 status;

    if (driver == NULL)
    {
        return GPIO_DRIVER_INVALID_ARGUMENT;
    }

    status = XGpio_Initialize(
        &driver->instance,
        device_id);

    if (status != XST_SUCCESS)
    {
        return GPIO_DRIVER_INITIALIZATION_ERROR;
    }

    driver->channel = channel;

    /*
     * Direction bit:
     *   0 = output
     *   1 = input
     *
     * Only the three LED bits are configured as outputs.
     */
    XGpio_SetDataDirection(
        &driver->instance,
        driver->channel,
        ~APP_LED_ALL_MASK);

    GpioDriver_AllOff(driver);

    return GPIO_DRIVER_SUCCESS;
}

GpioDriverStatus GpioDriver_ShowTemperatureState(
    GpioDriver *driver,
    TemperatureState state)
{
    u32 output_value;

    if (driver == NULL)
    {
        return GPIO_DRIVER_INVALID_ARGUMENT;
    }

    switch (state)
    {
        case TEMPERATURE_STATE_NORMAL:
            output_value = APP_LED_GREEN_MASK;
            break;

        case TEMPERATURE_STATE_WARNING:
            output_value = APP_LED_YELLOW_MASK;
            break;

        case TEMPERATURE_STATE_CRITICAL:
            output_value = APP_LED_RED_MASK;
            break;

        default:
            return GPIO_DRIVER_INVALID_STATE;
    }

    XGpio_DiscreteWrite(
        &driver->instance,
        driver->channel,
        output_value);

    return GPIO_DRIVER_SUCCESS;
}

void GpioDriver_ShowError(
    GpioDriver *driver)
{
    if (driver == NULL)
    {
        return;
    }

    XGpio_DiscreteWrite(
        &driver->instance,
        driver->channel,
        APP_LED_RED_MASK);
}

void GpioDriver_AllOff(
    GpioDriver *driver)
{
    if (driver == NULL)
    {
        return;
    }

    XGpio_DiscreteWrite(
        &driver->instance,
        driver->channel,
        0U);
}
