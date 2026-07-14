#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H

#include "system_types.h"
#include "xiicps.h"
#include "xil_types.h"

namespace app
{

class EepromDriver
{
public:
    enum class Status : u8
    {
        Success = 0U,
        InvalidArgument,
        LookupError,
        InitializationError,
        ClockError,
        BusBusyTimeout,
        AddressSendError,
        ReadError,
        InvalidConfiguration
    };

    EepromDriver();

    EepromDriver(const EepromDriver &) = delete;
    EepromDriver &operator=(const EepromDriver &) = delete;

    Status initialize(
        u16 device_id,
        u16 slave_address,
        u32 serial_clock_hz);

    Status readConfiguration(SystemConfiguration &configuration);

private:
    Status waitUntilBusIdle();
    Status readBytes(u16 memory_address, u8 *data, u32 length);
    static char sanitizeSerialCharacter(u8 value);

    XIicPs instance_;
    u16 slave_address_;
    bool initialized_;
};

} // namespace app

#endif
