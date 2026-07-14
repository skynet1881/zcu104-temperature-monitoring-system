#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "system_types.h"
#include "xgpio.h"
#include "xil_types.h"

namespace app
{

class GpioDriver
{
public:
    enum class Status : u8
    {
        Success = 0U,
        InvalidArgument,
        InitializationError,
        InvalidState
    };

    GpioDriver();

    GpioDriver(const GpioDriver &) = delete;
    GpioDriver &operator=(const GpioDriver &) = delete;

    Status initialize(u16 device_id, u32 channel);
    Status showTemperatureState(TemperatureState state);
    void showError();
    void allOff();

private:
    XGpio instance_;
    u32 channel_;
    bool initialized_;
};

} // namespace app

#endif
