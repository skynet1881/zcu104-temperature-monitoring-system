#ifndef TEMPERATURE_DRIVER_H
#define TEMPERATURE_DRIVER_H

#include "xil_types.h"
#include "xscugic.h"

typedef enum
{
    TEMPERATURE_DRIVER_SUCCESS = 0,
    TEMPERATURE_DRIVER_INVALID_ARGUMENT,
    TEMPERATURE_DRIVER_INITIALIZATION_ERROR,
    TEMPERATURE_DRIVER_INTERRUPT_CONNECT_ERROR
} TemperatureDriverStatus;

typedef struct
{
    u32 output_scale;
    s32 temperature_x10;
    u32 raw_value;
    u32 status;
} TemperatureSample;

typedef struct
{
    UINTPTR base_address;

    /*
     * Incremented by the ISR. SystemManager remembers the last processed
     * value instead of clearing a shared Boolean flag. This avoids losing
     * an interrupt due to a check-and-clear race.
     */
    volatile u32 interrupt_sequence;
} TemperatureDriver;

TemperatureDriverStatus TemperatureDriver_Initialize(
    TemperatureDriver *driver,
    UINTPTR base_address);

TemperatureDriverStatus TemperatureDriver_ConfigureInterrupt(
    TemperatureDriver *driver,
    XScuGic *interrupt_controller,
    u32 interrupt_id,
    u8 priority,
    u8 trigger_type);

TemperatureDriverStatus TemperatureDriver_ReadSample(
    const TemperatureDriver *driver,
    TemperatureSample *sample);

u32 TemperatureDriver_GetInterruptSequence(
    const TemperatureDriver *driver);

void TemperatureDriver_AcknowledgeInterrupt(
    const TemperatureDriver *driver);

#endif
