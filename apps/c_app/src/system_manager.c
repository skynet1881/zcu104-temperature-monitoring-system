#include "system_manager.h"
#include <stddef.h>

#include "system_config.h"
#include "eeprom_driver.h"
#include "gpio_driver.h"
#include "system_types.h"
#include "temperature_driver.h"

#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xstatus.h"

typedef struct
{
    GpioDriver gpio;
    EepromDriver eeprom;
    TemperatureDriver temperature;
    XScuGic interrupt_controller;

    SystemConfiguration configuration;
    TemperatureState temperature_state;

    u32 last_processed_interrupt_sequence;
    u32 initialized;
} SystemContext;

static SystemContext system_context;

static SystemManagerStatus SystemManager_InitializeInterruptController(void)
{
    XScuGic_Config *configuration;
    s32 status;

    configuration = XScuGic_LookupConfig(
        APP_GIC_DEVICE_ID);

    if (configuration == NULL)
    {
        return SYSTEM_MANAGER_GIC_INITIALIZATION_ERROR;
    }

    status = XScuGic_CfgInitialize(
        &system_context.interrupt_controller,
        configuration,
        configuration->CpuBaseAddress);

    if (status != XST_SUCCESS)
    {
        return SYSTEM_MANAGER_GIC_INITIALIZATION_ERROR;
    }

    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(
        XIL_EXCEPTION_ID_INT,
        (Xil_ExceptionHandler)XScuGic_InterruptHandler,
        &system_context.interrupt_controller);

    return SYSTEM_MANAGER_SUCCESS;
}

static u32 SystemManager_GetExpectedScale(
    HardwareRevision revision)
{
    if (revision == HARDWARE_REVISION_A)
    {
        /* Rev-A: one raw digit represents one degree Celsius. */
        return 1U;
    }

    /* Rev-B: one raw digit represents 0.1 degree Celsius. */
    return 10U;
}

static s32 SystemManager_ConvertRawToX10(
    u32 raw_value,
    HardwareRevision revision)
{
    if (revision == HARDWARE_REVISION_A)
    {
        return (s32)(raw_value * 10U);
    }

    return (s32)raw_value;
}

static TemperatureState SystemManager_DetermineTemperatureState(
    s32 temperature_x10)
{
    if ((temperature_x10 < APP_CRITICAL_LOW_X10) ||
        (temperature_x10 >= APP_CRITICAL_HIGH_X10))
    {
        return TEMPERATURE_STATE_CRITICAL;
    }

    if (temperature_x10 >= APP_WARNING_X10)
    {
        return TEMPERATURE_STATE_WARNING;
    }

    return TEMPERATURE_STATE_NORMAL;
}

static const char *SystemManager_GetStateName(
    TemperatureState state)
{
    switch (state)
    {
        case TEMPERATURE_STATE_NORMAL:
            return "NORMAL";

        case TEMPERATURE_STATE_WARNING:
            return "WARNING";

        case TEMPERATURE_STATE_CRITICAL:
            return "CRITICAL";

        default:
            return "UNKNOWN";
    }
}

static void SystemManager_PrintTemperature(
    s32 temperature_x10)
{
    s32 absolute_temperature;
    s32 whole;
    s32 fraction;

    if (temperature_x10 < 0)
    {
        absolute_temperature = -temperature_x10;
        whole = absolute_temperature / 10;
        fraction = absolute_temperature % 10;

        xil_printf("-%d.%d", (int)whole, (int)fraction);
    }
    else
    {
        whole = temperature_x10 / 10;
        fraction = temperature_x10 % 10;

        xil_printf("%d.%d", (int)whole, (int)fraction);
    }
}

static SystemManagerStatus SystemManager_ProcessTemperatureSample(
    u32 verify_scale)
{
    TemperatureDriverStatus temperature_status;
    GpioDriverStatus gpio_status;
    TemperatureSample sample;
    TemperatureState new_state;
    s32 converted_temperature_x10;
    u32 expected_scale;

    temperature_status = TemperatureDriver_ReadSample(
        &system_context.temperature,
        &sample);

    if (temperature_status != TEMPERATURE_DRIVER_SUCCESS)
    {
        return SYSTEM_MANAGER_TEMPERATURE_READ_ERROR;
    }

    expected_scale = SystemManager_GetExpectedScale(
        system_context.configuration.hardware_revision);

    if ((verify_scale != 0U) &&
        (sample.output_scale != expected_scale))
    {
        xil_printf(
            "ERROR: sensor scale mismatch: EEPROM expects %lu, IP reports %lu\r\n",
            (unsigned long)expected_scale,
            (unsigned long)sample.output_scale);

        return SYSTEM_MANAGER_SENSOR_SCALE_MISMATCH;
    }

    /*
     * Classification is deliberately performed from the raw sensor value,
     * so both hardware revisions exercise their required conversion:
     *
     * Rev-A raw 10  -> 10.0 C -> temperature_x10 = 100
     * Rev-B raw 100 -> 10.0 C -> temperature_x10 = 100
     */
    converted_temperature_x10 =
        SystemManager_ConvertRawToX10(
            sample.raw_value,
            system_context.configuration.hardware_revision);

    new_state = SystemManager_DetermineTemperatureState(
        converted_temperature_x10);

    /*
     * Every IRQ means "a new sample exists".
     * It does not necessarily mean "the system state changed".
     * GPIO and UART are updated only when the classified state changes.
     */
    if (new_state != system_context.temperature_state)
    {
        gpio_status = GpioDriver_ShowTemperatureState(
            &system_context.gpio,
            new_state);

        if (gpio_status != GPIO_DRIVER_SUCCESS)
        {
            return SYSTEM_MANAGER_GPIO_UPDATE_ERROR;
        }

        xil_printf("State change: %s -> %s, temperature=",
            SystemManager_GetStateName(
                system_context.temperature_state),
            SystemManager_GetStateName(new_state));

        SystemManager_PrintTemperature(
            converted_temperature_x10);

        xil_printf(
            " C, raw=%lu, irq_sequence=%lu\r\n",
            (unsigned long)sample.raw_value,
            (unsigned long)TemperatureDriver_GetInterruptSequence(
                &system_context.temperature));

        system_context.temperature_state = new_state;
    }

    return SYSTEM_MANAGER_SUCCESS;
}

SystemManagerStatus SystemManager_Initialize(void)
{
    GpioDriverStatus gpio_status;
    EepromDriverStatus eeprom_status;
    TemperatureDriverStatus temperature_status;
    SystemManagerStatus system_status;

    system_context.initialized = 0U;
    system_context.temperature_state = TEMPERATURE_STATE_UNKNOWN;
    system_context.last_processed_interrupt_sequence = 0U;

    gpio_status = GpioDriver_Initialize(
        &system_context.gpio,
        APP_GPIO_DEVICE_ID,
        APP_GPIO_CHANNEL);

    if (gpio_status != GPIO_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: GPIO initialization failed: %d\r\n",
            (int)gpio_status);

        return SYSTEM_MANAGER_GPIO_INITIALIZATION_ERROR;
    }

    eeprom_status = EepromDriver_Initialize(
        &system_context.eeprom,
        APP_I2C_DEVICE_ID,
        APP_EEPROM_I2C_ADDRESS,
        APP_EEPROM_I2C_CLOCK_HZ);

    if (eeprom_status != EEPROM_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: EEPROM initialization failed: %d\r\n",
            (int)eeprom_status);

        GpioDriver_ShowError(&system_context.gpio);
        return SYSTEM_MANAGER_EEPROM_INITIALIZATION_ERROR;
    }

    eeprom_status = EepromDriver_ReadConfiguration(
        &system_context.eeprom,
        &system_context.configuration);

    if (eeprom_status != EEPROM_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: EEPROM configuration read failed: %d\r\n",
            (int)eeprom_status);
        
        system_context.configuration.hardware_revision = HARDWARE_REVISION_A;
        //GpioDriver_ShowError(&system_context.gpio);
        //return SYSTEM_MANAGER_CONFIGURATION_READ_ERROR;
    }

    xil_printf(
        "Configuration: revision=%d, serial=%s\r\n",
        (int)system_context.configuration.hardware_revision,
        system_context.configuration.serial_number);

    temperature_status = TemperatureDriver_Initialize(
        &system_context.temperature,
        APP_TEMPERATURE_BASE_ADDRESS);

    if (temperature_status != TEMPERATURE_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: temperature driver initialization failed: %d\r\n",
            (int)temperature_status);

        GpioDriver_ShowError(&system_context.gpio);
        return SYSTEM_MANAGER_TEMPERATURE_INITIALIZATION_ERROR;
    }

    system_status = SystemManager_InitializeInterruptController();
    if (system_status != SYSTEM_MANAGER_SUCCESS)
    {
        xil_printf(
            "ERROR: GIC initialization failed: %d\r\n",
            (int)system_status);

        GpioDriver_ShowError(&system_context.gpio);
        return system_status;
    }

    temperature_status = TemperatureDriver_ConfigureInterrupt(
        &system_context.temperature,
        &system_context.interrupt_controller,
        APP_TEMPERATURE_INTERRUPT_ID,
        APP_TEMPERATURE_IRQ_PRIORITY,
        APP_TEMPERATURE_IRQ_TRIGGER_TYPE);

    if (temperature_status != TEMPERATURE_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: temperature IRQ initialization failed: %d\r\n",
            (int)temperature_status);

        GpioDriver_ShowError(&system_context.gpio);
        return SYSTEM_MANAGER_INTERRUPT_INITIALIZATION_ERROR;
    }

    /*
     * Enable CPU IRQ exceptions only after:
     *   1. GIC is initialized,
     *   2. handler is registered,
     *   3. peripheral interrupt is connected and acknowledged.
     */
    Xil_ExceptionEnable();

    /*
     * Initialize the LED immediately from the current sample rather than
     * waiting for the first interrupt.
     */
    system_status = SystemManager_ProcessTemperatureSample(1U);
    if (system_status != SYSTEM_MANAGER_SUCCESS)
    {
        GpioDriver_ShowError(&system_context.gpio);
        return system_status;
    }

    system_context.last_processed_interrupt_sequence =
        TemperatureDriver_GetInterruptSequence(
            &system_context.temperature);

    system_context.initialized = 1U;

    xil_printf(
        "Temperature monitoring started: IRQ ID=%lu, level-sensitive\r\n",
        (unsigned long)APP_TEMPERATURE_INTERRUPT_ID);

    return SYSTEM_MANAGER_SUCCESS;
}

SystemManagerStatus SystemManager_Process(void)
{
    u32 current_interrupt_sequence;

    if (system_context.initialized == 0U)
    {
        return SYSTEM_MANAGER_INVALID_ARGUMENT;
    }

    current_interrupt_sequence =
        TemperatureDriver_GetInterruptSequence(
            &system_context.temperature);

    if (current_interrupt_sequence ==
        system_context.last_processed_interrupt_sequence)
    {
        return SYSTEM_MANAGER_SUCCESS;
    }

    // monitoring system has received a new temperature sample interrupt
    system_context.last_processed_interrupt_sequence =
        current_interrupt_sequence;

    return SystemManager_ProcessTemperatureSample(0U);
}
