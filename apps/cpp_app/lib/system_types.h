#ifndef SYSTEM_TYPES_HPP
#define SYSTEM_TYPES_HPP

#include "xil_types.h"

namespace app
{

constexpr u32 kSystemSerialNumberLength = 7U;

enum class HardwareRevision : u8
{
    RevisionA = 0U,
    RevisionB = 1U
};

struct SystemConfiguration
{
    HardwareRevision hardware_revision;
    char serial_number[kSystemSerialNumberLength + 1U];
};

enum class TemperatureState : u8
{
    Unknown = 0U,
    Normal,
    Warning,
    Critical
};

} // namespace app

#endif
