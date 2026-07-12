#include "eeprom_driver.h"

#include <stddef.h>
#include <string.h>

#include "xiicps.h"
#include "xstatus.h"

#define EEPROM_DRIVER_MAX_ADDRESS_BYTES 2U

static XIicPs eeprom_i2c_instance;
static EepromDriverConfig eeprom_config;
static bool eeprom_driver_initialized;

static EepromDriverStatus EepromDriver_EncodeAddress(
    uint32_t address,
    uint8_t *buffer)
{
    if (buffer == NULL)
    {
        return EEPROM_DRIVER_INVALID_ARGUMENT;
    }

    if (eeprom_config.address_bytes == 1U)
    {
        buffer[0] = (uint8_t)(address & 0xFFU);
        return EEPROM_DRIVER_SUCCESS;
    }

    if (eeprom_config.address_bytes == 2U)
    {
        buffer[0] = (uint8_t)((address >> 8U) & 0xFFU);
        buffer[1] = (uint8_t)(address & 0xFFU);
        return EEPROM_DRIVER_SUCCESS;
    }

    return EEPROM_DRIVER_CONFIGURATION_ERROR;
}

EepromDriverStatus EepromDriver_Init(
    const EepromDriverConfig *config)
{
    XIicPs_Config *i2c_configuration;
    int status;

    if (config == NULL)
    {
        return EEPROM_DRIVER_INVALID_ARGUMENT;
    }

    if ((config->serial_clock_hz == 0U) ||
        (config->address_bytes == 0U) ||
        (config->address_bytes > EEPROM_DRIVER_MAX_ADDRESS_BYTES) ||
        (config->serial_number_length == 0U) ||
        (config->serial_number_length >
            EEPROM_DRIVER_MAX_SERIAL_LENGTH))
    {
        return EEPROM_DRIVER_CONFIGURATION_ERROR;
    }

    eeprom_driver_initialized = false;
    eeprom_config = *config;

    i2c_configuration = XIicPs_LookupConfig(
        config->i2c_device_id);

    if (i2c_configuration == NULL)
    {
        return EEPROM_DRIVER_CONFIGURATION_ERROR;
    }

    status = XIicPs_CfgInitialize(
        &eeprom_i2c_instance,
        i2c_configuration,
        i2c_configuration->BaseAddress);

    if (status != XST_SUCCESS)
    {
        return EEPROM_DRIVER_INITIALIZATION_ERROR;
    }

    status = XIicPs_SelfTest(&eeprom_i2c_instance);

    if (status != XST_SUCCESS)
    {
        return EEPROM_DRIVER_SELF_TEST_FAILED;
    }

    XIicPs_SetSClk(
        &eeprom_i2c_instance,
        config->serial_clock_hz);

    eeprom_driver_initialized = true;

    return EEPROM_DRIVER_SUCCESS;
}

bool EepromDriver_IsInitialized(void)
{
    return eeprom_driver_initialized;
}

EepromDriverStatus EepromDriver_Read(
    uint32_t address,
    uint8_t *data,
    uint32_t length)
{
    uint8_t address_buffer[EEPROM_DRIVER_MAX_ADDRESS_BYTES];
    EepromDriverStatus encode_status;
    int status;

    if (!eeprom_driver_initialized)
    {
        return EEPROM_DRIVER_NOT_INITIALIZED;
    }

    if ((data == NULL) || (length == 0U))
    {
        return EEPROM_DRIVER_INVALID_ARGUMENT;
    }

    encode_status = EepromDriver_EncodeAddress(
        address,
        address_buffer);

    if (encode_status != EEPROM_DRIVER_SUCCESS)
    {
        return encode_status;
    }

    status = XIicPs_MasterSendPolled(
        &eeprom_i2c_instance,
        address_buffer,
        (int)eeprom_config.address_bytes,
        eeprom_config.slave_address);

    if (status != XST_SUCCESS)
    {
        return EEPROM_DRIVER_READ_ERROR;
    }

    while (XIicPs_BusIsBusy(&eeprom_i2c_instance) != 0)
    {
        /* Wait until the EEPROM address transfer has completed. */
    }

    status = XIicPs_MasterRecvPolled(
        &eeprom_i2c_instance,
        data,
        (int)length,
        eeprom_config.slave_address);

    if (status != XST_SUCCESS)
    {
        return EEPROM_DRIVER_READ_ERROR;
    }

    while (XIicPs_BusIsBusy(&eeprom_i2c_instance) != 0)
    {
        /* Wait until the EEPROM read has completed. */
    }

    return EEPROM_DRIVER_SUCCESS;
}

EepromDriverStatus EepromDriver_ReadConfiguration(
    EepromConfiguration *configuration)
{
    uint8_t revision_value;
    EepromDriverStatus status;

    if (!eeprom_driver_initialized)
    {
        return EEPROM_DRIVER_NOT_INITIALIZED;
    }

    if (configuration == NULL)
    {
        return EEPROM_DRIVER_INVALID_ARGUMENT;
    }

    memset(configuration, 0, sizeof(*configuration));

    status = EepromDriver_Read(
        eeprom_config.revision_address,
        &revision_value,
        sizeof(revision_value));

    if (status != EEPROM_DRIVER_SUCCESS)
    {
        return status;
    }

    if (revision_value == (uint8_t)HARDWARE_REVISION_A)
    {
        configuration->hardware_revision = HARDWARE_REVISION_A;
    }
    else if (revision_value == (uint8_t)HARDWARE_REVISION_B)
    {
        configuration->hardware_revision = HARDWARE_REVISION_B;
    }
    else
    {
        return EEPROM_DRIVER_INVALID_CONTENT;
    }

    status = EepromDriver_Read(
        eeprom_config.serial_number_address,
        (uint8_t *)configuration->serial_number,
        eeprom_config.serial_number_length);

    if (status != EEPROM_DRIVER_SUCCESS)
    {
        return status;
    }

    configuration->serial_number[
        eeprom_config.serial_number_length] = '\0';

    return EEPROM_DRIVER_SUCCESS;
}
