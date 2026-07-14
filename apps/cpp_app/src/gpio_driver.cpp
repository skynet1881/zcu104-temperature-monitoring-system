#include "gpio_driver.hpp"

#include "system_config.hpp"
#include "xstatus.h"

namespace app
{

GpioDriver::GpioDriver()
    : instance_{},
      channel_(0U),
      initialized_(false)
{
}

GpioDriver::Status GpioDriver::initialize(
    u16 device_id,
    u32 channel)
{
    const s32 status = XGpio_Initialize(&instance_, device_id);
    if (status != XST_SUCCESS)
    {
        return Status::InitializationError;
    }

    channel_ = channel;
    initialized_ = true;

    /* Direction bit: 0 = output, 1 = input. */
    XGpio_SetDataDirection(
        &instance_,
        channel_,
        ~config::kLedAllMask);

    allOff();
    return Status::Success;
}

GpioDriver::Status GpioDriver::showTemperatureState(
    TemperatureState state)
{
    if (!initialized_)
    {
        return Status::InvalidArgument;
    }

    u32 output_value = 0U;

    switch (state)
    {
        case TemperatureState::Normal:
            output_value = config::kLedGreenMask;
            break;

        case TemperatureState::Warning:
            output_value = config::kLedYellowMask;
            break;

        case TemperatureState::Critical:
            output_value = config::kLedRedMask;
            break;

        default:
            return Status::InvalidState;
    }

    XGpio_DiscreteWrite(&instance_, channel_, output_value);
    return Status::Success;
}

void GpioDriver::showError()
{
    if (initialized_)
    {
        XGpio_DiscreteWrite(
            &instance_,
            channel_,
            config::kLedRedMask);
    }
}

void GpioDriver::allOff()
{
    if (initialized_)
    {
        XGpio_DiscreteWrite(&instance_, channel_, 0U);
    }
}

} // namespace app
