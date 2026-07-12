#include "gpio_driver.h"

#include <stddef.h>

#include "xgpio.h"
#include "xstatus.h"

#define TEMPERATURE_CRITICAL_LOW_X10   50
#define TEMPERATURE_WARNING_X10        850
#define TEMPERATURE_CRITICAL_HIGH_X10  1050

static XGpio gpio_instance;
static GpioDriverConfig gpio_config;
static bool gpio_driver_initialized;

GpioDriverStatus GpioDriver_Init(
    const GpioDriverConfig *config)
{
    int status;

    if (config == NULL)
    {
        return GPIO_DRIVER_INVALID_ARGUMENT;
    }

    if ((config->channel == 0U) ||
        (config->output_mask == 0U) ||
        (config->green_led_mask == 0U) ||
        (config->yellow_led_mask == 0U) ||
        (config->red_led_mask == 0U))
    {
        return GPIO_DRIVER_INVALID_ARGUMENT;
    }

    gpio_driver_initialized = false;
    gpio_config = *config;

    status = XGpio_Initialize(
        &gpio_instance,
        gpio_config.device_id);

    if (status != XST_SUCCESS)
    {
        return GPIO_DRIVER_INITIALIZATION_ERROR;
    }

    /*
     * A zero bit configures the corresponding GPIO pin as an output.
     */
    XGpio_SetDataDirection(
        &gpio_instance,
        gpio_config.channel,
        ~gpio_config.output_mask);

    XGpio_DiscreteWrite(
        &gpio_instance,
        gpio_config.channel,
        0U);

    gpio_driver_initialized = true;

    return GPIO_DRIVER_SUCCESS;
}

bool GpioDriver_IsInitialized(void)
{
    return gpio_driver_initialized;
}

GpioDriverStatus GpioDriver_SetLedPattern(
    uint32_t pattern)
{
    if (!gpio_driver_initialized)
    {
        return GPIO_DRIVER_NOT_INITIALIZED;
    }

    XGpio_DiscreteWrite(
        &gpio_instance,
        gpio_config.channel,
        pattern & gpio_config.output_mask);

    return GPIO_DRIVER_SUCCESS;
}

GpioDriverStatus GpioDriver_ShowTemperature(
    int32_t temperature_x10)
{
    uint32_t pattern;

    if (!gpio_driver_initialized)
    {
        return GPIO_DRIVER_NOT_INITIALIZED;
    }

    if ((temperature_x10 < TEMPERATURE_CRITICAL_LOW_X10) ||
        (temperature_x10 >= TEMPERATURE_CRITICAL_HIGH_X10))
    {
        pattern = gpio_config.red_led_mask;
    }
    else if (temperature_x10 >= TEMPERATURE_WARNING_X10)
    {
        pattern = gpio_config.yellow_led_mask;
    }
    else
    {
        pattern = gpio_config.green_led_mask;
    }

    return GpioDriver_SetLedPattern(pattern);
}

GpioDriverStatus GpioDriver_ShowError(void)
{
    return GpioDriver_SetLedPattern(gpio_config.red_led_mask);
}

GpioDriverStatus GpioDriver_AllOff(void)
{
    return GpioDriver_SetLedPattern(0U);
}
