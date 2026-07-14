#ifndef TEMPERATURE_DRIVER_H
#define TEMPERATURE_DRIVER_H

#include "xil_types.h"
#include "xscugic.h"

namespace app
{

class TemperatureDriver
{
public:
    enum class Status : u8
    {
        Success = 0U,
        InvalidArgument,
        InitializationError,
        InterruptConnectError
    };

    struct Sample
    {
        u32 output_scale;
        s32 temperature_x10;
        u32 raw_value;
        u32 status;
    };

    TemperatureDriver();

    TemperatureDriver(const TemperatureDriver &) = delete;
    TemperatureDriver &operator=(const TemperatureDriver &) = delete;

    Status initialize(UINTPTR base_address);

    Status configureInterrupt(
        XScuGic &interrupt_controller,
        u32 interrupt_id,
        u8 priority,
        u8 trigger_type);

    Status readSample(Sample &sample) const;

    u32 interruptSequence() const;
    void acknowledgeInterrupt() const;

private:
    static void interruptHandler(void *callback_reference);

    UINTPTR base_address_;
    volatile u32 interrupt_sequence_;
};

} // namespace app

#endif
