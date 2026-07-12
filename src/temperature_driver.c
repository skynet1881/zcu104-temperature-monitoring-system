#include "temperature_driver.h"

#include <stddef.h>

#include "xil_io.h"

#define TEMPERATURE_REGISTER_RAW_VALUE_OFFSET 0x08U
#define TEMPERATURE_REGISTER_STATUS_OFFSET    0x0CU

/*
 * STATUS[0] indicates that a new sample is available.
 * Writing one to STATUS[0] is expected to clear the pending condition.
 */
#define TEMPERATURE_STATUS_DATA_READY_MASK (1UL << 0U)

static uintptr_t temperature_base_address;
static bool temperature_driver_initialized;

static uint32_t TemperatureDriver_ReadRegister(uint32_t offset)
{
    return Xil_In32(temperature_base_address + (uintptr_t)offset);
}

static void TemperatureDriver_WriteRegister(
    uint32_t offset,
    uint32_t value)
{
    Xil_Out32(
        temperature_base_address + (uintptr_t)offset,
        value);
}

TemperatureDriverStatus TemperatureDriver_Init(uintptr_t base_address)
{
    TemperatureDriverStatus status;

    if (base_address == (uintptr_t)0U)
    {
        temperature_base_address = (uintptr_t)0U;
        temperature_driver_initialized = false;
        return TEMPERATURE_DRIVER_INVALID_ARGUMENT;
    }

    temperature_base_address = base_address;
    temperature_driver_initialized = true;

    status = TemperatureDriver_SelfTest();

    if (status != TEMPERATURE_DRIVER_SUCCESS)
    {
        temperature_driver_initialized = false;
    }

    return status;
}

bool TemperatureDriver_IsInitialized(void)
{
    return temperature_driver_initialized;
}

TemperatureDriverStatus TemperatureDriver_GetDataReady(bool *is_ready)
{
    uint32_t status_register;

    if (!temperature_driver_initialized)
    {
        return TEMPERATURE_DRIVER_NOT_INITIALIZED;
    }

    if (is_ready == NULL)
    {
        return TEMPERATURE_DRIVER_INVALID_ARGUMENT;
    }

    status_register = TemperatureDriver_ReadRegister(
        TEMPERATURE_REGISTER_STATUS_OFFSET);

    *is_ready =
        (status_register & TEMPERATURE_STATUS_DATA_READY_MASK) != 0U;

    return TEMPERATURE_DRIVER_SUCCESS;
}

TemperatureDriverStatus TemperatureDriver_Read(
    TemperatureSample *sample)
{
    uint32_t status_register;

    if (!temperature_driver_initialized)
    {
        return TEMPERATURE_DRIVER_NOT_INITIALIZED;
    }

    if (sample == NULL)
    {
        return TEMPERATURE_DRIVER_INVALID_ARGUMENT;
    }

    status_register = TemperatureDriver_ReadRegister(
        TEMPERATURE_REGISTER_STATUS_OFFSET);

    if ((status_register & TEMPERATURE_STATUS_DATA_READY_MASK) == 0U)
    {
        return TEMPERATURE_DRIVER_DATA_NOT_READY;
    }

    sample->raw_value = TemperatureDriver_ReadRegister(
        TEMPERATURE_REGISTER_RAW_VALUE_OFFSET);

    sample->status_register = status_register;

    return TEMPERATURE_DRIVER_SUCCESS;
}

TemperatureDriverStatus TemperatureDriver_AcknowledgeInterrupt(void)
{
    if (!temperature_driver_initialized)
    {
        return TEMPERATURE_DRIVER_NOT_INITIALIZED;
    }

    TemperatureDriver_WriteRegister(
        TEMPERATURE_REGISTER_STATUS_OFFSET,
        TEMPERATURE_STATUS_DATA_READY_MASK);

    return TEMPERATURE_DRIVER_SUCCESS;
}

TemperatureDriverStatus TemperatureDriver_SelfTest(void)
{
    if (!temperature_driver_initialized)
    {
        return TEMPERATURE_DRIVER_NOT_INITIALIZED;
    }

    /*
     * Reading the status register verifies that the AXI interface is
     * addressable. An AXI decode failure may still cause a processor exception.
     */
    (void)TemperatureDriver_ReadRegister(
        TEMPERATURE_REGISTER_STATUS_OFFSET);

    return TEMPERATURE_DRIVER_SUCCESS;
}
