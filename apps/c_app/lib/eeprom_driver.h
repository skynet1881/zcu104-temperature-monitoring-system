#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H

#include "system_types.h"
#include "xiicps.h"
#include "xil_types.h"

typedef enum
{
    EEPROM_DRIVER_SUCCESS = 0,
    EEPROM_DRIVER_INVALID_ARGUMENT,
    EEPROM_DRIVER_LOOKUP_ERROR,
    EEPROM_DRIVER_INITIALIZATION_ERROR,
    EEPROM_DRIVER_CLOCK_ERROR,
    EEPROM_DRIVER_BUS_BUSY_TIMEOUT,
    EEPROM_DRIVER_ADDRESS_SEND_ERROR,
    EEPROM_DRIVER_READ_ERROR,
    EEPROM_DRIVER_INVALID_CONFIGURATION
} EepromDriverStatus;

typedef struct
{
    XIicPs instance;
    u16 slave_address;
} EepromDriver;

EepromDriverStatus EepromDriver_Initialize(
    EepromDriver *driver,
    u16 device_id,
    u16 slave_address,
    u32 serial_clock_hz);

EepromDriverStatus EepromDriver_ReadConfiguration(
    EepromDriver *driver,
    SystemConfiguration *configuration);

#endif
