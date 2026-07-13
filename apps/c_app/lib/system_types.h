#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

#include "xil_types.h"

#define SYSTEM_SERIAL_NUMBER_LENGTH 7U

typedef enum
{
    HARDWARE_REVISION_A = 0,
    HARDWARE_REVISION_B = 1
} HardwareRevision;

typedef struct
{
    HardwareRevision hardware_revision;
    char serial_number[SYSTEM_SERIAL_NUMBER_LENGTH + 1U];
} SystemConfiguration;

typedef enum
{
    TEMPERATURE_STATE_UNKNOWN = 0,
    TEMPERATURE_STATE_NORMAL,
    TEMPERATURE_STATE_WARNING,
    TEMPERATURE_STATE_CRITICAL
} TemperatureState;

#endif
