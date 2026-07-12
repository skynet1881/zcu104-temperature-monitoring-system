#include "system_manager.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "eeprom_driver.h"
#include "gpio_driver.h"
#include "sleep.h"
#include "system_config.h"
#include "temperature_driver.h"
#include "xil_printf.h"

typedef enum
{
    SYSTEM_STATE_WAIT_FOR_SAMPLE = 0,
    SYSTEM_STATE_READ_TEMPERATURE,
    SYSTEM_STATE_UPDATE_GPIO,
    SYSTEM_STATE_ERROR
} SystemState;

typedef struct
{
    SystemState state;
    EepromConfiguration configuration;
    TemperatureSample sample;
    int32_t temperature_x10;
    uint32_t sample_count;
    uint32_t error_count;
} SystemManagerContext;

static SystemManagerContext system_context;

static const char *SystemManager_GetRevisionName(
    HardwareRevision revision)
{
    if (revision == HARDWARE_REVISION_A)
    {
        return "Rev-A";
    }

    if (revision == HARDWARE_REVISION_B)
    {
        return "Rev-B";
    }

    return "Unknown";
}

static SystemManagerStatus SystemManager_ConvertTemperature(
    uint32_t raw_value,
    HardwareRevision revision,
    int32_t *temperature_x10)
{
    if (temperature_x10 == NULL)
    {
        return SYSTEM_MANAGER_TEMPERATURE_CONVERSION_ERROR;
    }

    if (revision == HARDWARE_REVISION_A)
    {
        /*
         * Rev-A resolution: 1 degree Celsius per digit.
         * Convert whole degrees into the common 0.1 degree representation.
         */
        if (raw_value > ((uint32_t)INT32_MAX / 10U))
        {
            return SYSTEM_MANAGER_TEMPERATURE_CONVERSION_ERROR;
        }

        *temperature_x10 = (int32_t)(raw_value * 10U);
        return SYSTEM_MANAGER_SUCCESS;
    }

    if (revision == HARDWARE_REVISION_B)
    {
        /*
         * Rev-B resolution: 0.1 degree Celsius per digit.
         * The raw value is already represented in tenths of a degree.
         */
        if (raw_value > (uint32_t)INT32_MAX)
        {
            return SYSTEM_MANAGER_TEMPERATURE_CONVERSION_ERROR;
        }

        *temperature_x10 = (int32_t)raw_value;
        return SYSTEM_MANAGER_SUCCESS;
    }

    return SYSTEM_MANAGER_INVALID_HARDWARE_REVISION;
}

SystemManagerStatus SystemManager_Init(void)
{
    TemperatureDriverStatus temperature_status;
    EepromDriverStatus eeprom_status;
    GpioDriverStatus gpio_status;

    const EepromDriverConfig eeprom_configuration =
    {
        .i2c_device_id = SYSTEM_EEPROM_I2C_DEVICE_ID,
        .slave_address = SYSTEM_EEPROM_I2C_ADDRESS,
        .serial_clock_hz = SYSTEM_EEPROM_I2C_CLOCK_HZ,
        .address_bytes = SYSTEM_EEPROM_ADDRESS_BYTES,
        .revision_address = SYSTEM_EEPROM_REVISION_ADDRESS,
        .serial_number_address = SYSTEM_EEPROM_SERIAL_ADDRESS,
        .serial_number_length = SYSTEM_EEPROM_SERIAL_LENGTH
    };

    const GpioDriverConfig gpio_configuration =
    {
        .device_id = SYSTEM_GPIO_DEVICE_ID,
        .channel = SYSTEM_GPIO_LED_CHANNEL,
        .output_mask = SYSTEM_GPIO_LED_MASK,
        .green_led_mask = SYSTEM_GPIO_GREEN_LED_MASK,
        .yellow_led_mask = SYSTEM_GPIO_YELLOW_LED_MASK,
        .red_led_mask = SYSTEM_GPIO_RED_LED_MASK
    };

    memset(&system_context, 0, sizeof(system_context));
    system_context.state = SYSTEM_STATE_WAIT_FOR_SAMPLE;

    gpio_status = GpioDriver_Init(&gpio_configuration);

    if (gpio_status != GPIO_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: GPIO initialization failed: %d\r\n",
            (int)gpio_status);

        return SYSTEM_MANAGER_INITIALIZATION_ERROR;
    }

    (void)GpioDriver_AllOff();

    eeprom_status = EepromDriver_Init(&eeprom_configuration);

    if (eeprom_status != EEPROM_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: EEPROM initialization failed: %d\r\n",
            (int)eeprom_status);

        (void)GpioDriver_ShowError();
        return SYSTEM_MANAGER_INITIALIZATION_ERROR;
    }

    eeprom_status = EepromDriver_ReadConfiguration(
        &system_context.configuration);

    if (eeprom_status != EEPROM_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: EEPROM configuration read failed: %d\r\n",
            (int)eeprom_status);

        (void)GpioDriver_ShowError();
        return SYSTEM_MANAGER_INITIALIZATION_ERROR;
    }

    xil_printf(
        "Hardware revision: %s\r\n",
        SystemManager_GetRevisionName(
            system_context.configuration.hardware_revision));

    xil_printf(
        "Hardware serial: %s\r\n",
        system_context.configuration.serial_number);

    temperature_status = TemperatureDriver_Init(
        (uintptr_t)SYSTEM_TEMPERATURE_BASE_ADDRESS);

    if (temperature_status != TEMPERATURE_DRIVER_SUCCESS)
    {
        xil_printf(
            "ERROR: Temperature driver initialization failed: %d\r\n",
            (int)temperature_status);

        (void)GpioDriver_ShowError();
        return SYSTEM_MANAGER_INITIALIZATION_ERROR;
    }

    xil_printf("System manager initialized successfully\r\n");

    return SYSTEM_MANAGER_SUCCESS;
}

SystemManagerStatus SystemManager_Process(void)
{
    switch (system_context.state)
    {
        case SYSTEM_STATE_WAIT_FOR_SAMPLE:
        {
            bool is_ready = false;
            TemperatureDriverStatus status =
                TemperatureDriver_GetDataReady(&is_ready);

            if (status != TEMPERATURE_DRIVER_SUCCESS)
            {
                system_context.state = SYSTEM_STATE_ERROR;
                return SYSTEM_MANAGER_DRIVER_ERROR;
            }

            if (is_ready)
            {
                system_context.state =
                    SYSTEM_STATE_READ_TEMPERATURE;
            }

            break;
        }

        case SYSTEM_STATE_READ_TEMPERATURE:
        {
            TemperatureDriverStatus temperature_status;
            SystemManagerStatus conversion_status;

            temperature_status =
                TemperatureDriver_Read(&system_context.sample);

            if (temperature_status ==
                TEMPERATURE_DRIVER_DATA_NOT_READY)
            {
                system_context.state =
                    SYSTEM_STATE_WAIT_FOR_SAMPLE;

                break;
            }

            if (temperature_status != TEMPERATURE_DRIVER_SUCCESS)
            {
                system_context.state = SYSTEM_STATE_ERROR;
                return SYSTEM_MANAGER_DRIVER_ERROR;
            }

            conversion_status = SystemManager_ConvertTemperature(
                system_context.sample.raw_value,
                system_context.configuration.hardware_revision,
                &system_context.temperature_x10);

            if (conversion_status != SYSTEM_MANAGER_SUCCESS)
            {
                system_context.state = SYSTEM_STATE_ERROR;
                return conversion_status;
            }

            temperature_status =
                TemperatureDriver_AcknowledgeInterrupt();

            if (temperature_status != TEMPERATURE_DRIVER_SUCCESS)
            {
                system_context.state = SYSTEM_STATE_ERROR;
                return SYSTEM_MANAGER_DRIVER_ERROR;
            }

            system_context.sample_count++;

            xil_printf(
                "Sample %lu: raw=%lu, revision=%s, "
                "temperature=%ld.%01ld C\r\n",
                (unsigned long)system_context.sample_count,
                (unsigned long)system_context.sample.raw_value,
                SystemManager_GetRevisionName(
                    system_context.configuration.hardware_revision),
                (long)(system_context.temperature_x10 / 10),
                (long)(system_context.temperature_x10 < 0
                    ? -(system_context.temperature_x10 % 10)
                    : (system_context.temperature_x10 % 10)));

            system_context.state = SYSTEM_STATE_UPDATE_GPIO;
            break;
        }

        case SYSTEM_STATE_UPDATE_GPIO:
        {
            GpioDriverStatus status =
                GpioDriver_ShowTemperature(
                    system_context.temperature_x10);

            if (status != GPIO_DRIVER_SUCCESS)
            {
                system_context.state = SYSTEM_STATE_ERROR;
                return SYSTEM_MANAGER_DRIVER_ERROR;
            }

            system_context.state = SYSTEM_STATE_WAIT_FOR_SAMPLE;
            break;
        }

        case SYSTEM_STATE_ERROR:
        default:
        {
            system_context.error_count++;

            xil_printf(
                "System error, count=%lu\r\n",
                (unsigned long)system_context.error_count);

            (void)GpioDriver_ShowError();

            system_context.state = SYSTEM_STATE_WAIT_FOR_SAMPLE;

            return SYSTEM_MANAGER_DRIVER_ERROR;
        }
    }

    return SYSTEM_MANAGER_SUCCESS;
}

void SystemManager_Run(void)
{
    for (;;)
    {
        (void)SystemManager_Process();
        usleep(SYSTEM_MANAGER_POLL_INTERVAL_US);
    }
}
