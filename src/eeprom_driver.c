#include "eeprom_driver.h"
#include <stddef.h>

#include "system_config.h"
#include "xstatus.h"

#define EEPROM_ADDRESS_SIZE_BYTES  2U
#define EEPROM_BUS_TIMEOUT_LOOPS   1000000U

static EepromDriverStatus EepromDriver_WaitUntilBusIdle(
    EepromDriver *driver)
{
    u32 timeout = EEPROM_BUS_TIMEOUT_LOOPS;

    while (XIicPs_BusIsBusy(&driver->instance) != 0)
    {
        if (timeout == 0U)
        {
            return EEPROM_DRIVER_BUS_BUSY_TIMEOUT;
        }

        timeout--;
    }

    return EEPROM_DRIVER_SUCCESS;
}

static EepromDriverStatus EepromDriver_ReadBytes(
    EepromDriver *driver,
    u16 memory_address,
    u8 *data,
    u32 length)
{
    u8 address_bytes[EEPROM_ADDRESS_SIZE_BYTES];
    s32 status;
    EepromDriverStatus wait_status;

    if ((driver == NULL) || (data == NULL) || (length == 0U))
    {
        return EEPROM_DRIVER_INVALID_ARGUMENT;
    }

    wait_status = EepromDriver_WaitUntilBusIdle(driver);
    if (wait_status != EEPROM_DRIVER_SUCCESS)
    {
        return wait_status;
    }

    address_bytes[0] = (u8)((memory_address >> 8U) & 0xFFU);
    address_bytes[1] = (u8)(memory_address & 0xFFU);

    /*
     * Hold the bus after sending the EEPROM memory address so the following
     * receive operation uses a repeated START.
     */
    XIicPs_SetOptions(
        &driver->instance,
        XIICPS_REP_START_OPTION);

    status = XIicPs_MasterSendPolled(
        &driver->instance,
        address_bytes,
        EEPROM_ADDRESS_SIZE_BYTES,
        driver->slave_address);

    if (status != XST_SUCCESS)
    {
        XIicPs_ClearOptions(
            &driver->instance,
            XIICPS_REP_START_OPTION);

        return EEPROM_DRIVER_ADDRESS_SEND_ERROR;
    }

    status = XIicPs_MasterRecvPolled(
        &driver->instance,
        data,
        length,
        driver->slave_address);

    XIicPs_ClearOptions(
        &driver->instance,
        XIICPS_REP_START_OPTION);

    if (status != XST_SUCCESS)
    {
        return EEPROM_DRIVER_READ_ERROR;
    }

    return EepromDriver_WaitUntilBusIdle(driver);
}

static char EepromDriver_SanitizeSerialCharacter(u8 value)
{
    if ((value >= (u8)' ') && (value <= (u8)'~'))
    {
        return (char)value;
    }

    return '?';
}

EepromDriverStatus EepromDriver_Initialize(
    EepromDriver *driver,
    u16 device_id,
    u16 slave_address,
    u32 serial_clock_hz)
{
    XIicPs_Config *configuration;
    s32 status;

    if (driver == NULL)
    {
        return EEPROM_DRIVER_INVALID_ARGUMENT;
    }

    configuration = XIicPs_LookupConfig(device_id);
    if (configuration == NULL)
    {
        return EEPROM_DRIVER_LOOKUP_ERROR;
    }

    status = XIicPs_CfgInitialize(
        &driver->instance,
        configuration,
        configuration->BaseAddress);

    if (status != XST_SUCCESS)
    {
        return EEPROM_DRIVER_INITIALIZATION_ERROR;
    }

    status = XIicPs_SetSClk(
        &driver->instance,
        serial_clock_hz);

    if (status != XST_SUCCESS)
    {
        return EEPROM_DRIVER_CLOCK_ERROR;
    }

    driver->slave_address = slave_address;

    return EEPROM_DRIVER_SUCCESS;
}

EepromDriverStatus EepromDriver_ReadConfiguration(
    EepromDriver *driver,
    SystemConfiguration *configuration)
{
    u8 revision;
    u8 serial[SYSTEM_SERIAL_NUMBER_LENGTH];
    u32 index;
    EepromDriverStatus status;

    if ((driver == NULL) || (configuration == NULL))
    {
        return EEPROM_DRIVER_INVALID_ARGUMENT;
    }

    status = EepromDriver_ReadBytes(
        driver,
        APP_EEPROM_REVISION_OFFSET,
        &revision,
        1U);

    if (status != EEPROM_DRIVER_SUCCESS)
    {
        return status;
    }

    if (revision == (u8)HARDWARE_REVISION_A)
    {
        configuration->hardware_revision = HARDWARE_REVISION_A;
    }
    else if (revision == (u8)HARDWARE_REVISION_B)
    {
        configuration->hardware_revision = HARDWARE_REVISION_B;
    }
    else
    {
        return EEPROM_DRIVER_INVALID_CONFIGURATION;
    }

    status = EepromDriver_ReadBytes(
        driver,
        APP_EEPROM_SERIAL_OFFSET,
        serial,
        SYSTEM_SERIAL_NUMBER_LENGTH);

    if (status != EEPROM_DRIVER_SUCCESS)
    {
        return status;
    }

    for (index = 0U;
         index < SYSTEM_SERIAL_NUMBER_LENGTH;
         index++)
    {
        configuration->serial_number[index] =
            EepromDriver_SanitizeSerialCharacter(serial[index]);
    }

    configuration->serial_number[SYSTEM_SERIAL_NUMBER_LENGTH] = '\0';

    return EEPROM_DRIVER_SUCCESS;
}
