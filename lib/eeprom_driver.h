#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#define EEPROM_DRIVER_MAX_SERIAL_LENGTH 16U

typedef enum
{
    HARDWARE_REVISION_A = 0,
    HARDWARE_REVISION_B = 1
} HardwareRevision;

typedef enum
{
    EEPROM_DRIVER_SUCCESS = 0,
    EEPROM_DRIVER_NOT_INITIALIZED,
    EEPROM_DRIVER_INVALID_ARGUMENT,
    EEPROM_DRIVER_CONFIGURATION_ERROR,
    EEPROM_DRIVER_INITIALIZATION_ERROR,
    EEPROM_DRIVER_SELF_TEST_FAILED,
    EEPROM_DRIVER_READ_ERROR,
    EEPROM_DRIVER_INVALID_CONTENT
} EepromDriverStatus;

typedef struct
{
    uint16_t i2c_device_id;
    uint8_t slave_address;
    uint32_t serial_clock_hz;
    uint8_t address_bytes;
    uint32_t revision_address;
    uint32_t serial_number_address;
    uint8_t serial_number_length;
} EepromDriverConfig;

typedef struct
{
    HardwareRevision hardware_revision;
    char serial_number[EEPROM_DRIVER_MAX_SERIAL_LENGTH + 1U];
} EepromConfiguration;

EepromDriverStatus EepromDriver_Init(
    const EepromDriverConfig *config);

bool EepromDriver_IsInitialized(void);

EepromDriverStatus EepromDriver_Read(
    uint32_t address,
    uint8_t *data,
    uint32_t length);

EepromDriverStatus EepromDriver_ReadConfiguration(
    EepromConfiguration *configuration);

#endif /* EEPROM_DRIVER_H */
