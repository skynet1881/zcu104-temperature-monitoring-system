#include "temperature_driver.hpp"

#include "xil_io.h"
#include "xstatus.h"

namespace app
{
namespace
{

constexpr UINTPTR kScaleOffset = 0x00U;
constexpr UINTPTR kTemperatureX10Offset = 0x04U;
constexpr UINTPTR kRawOffset = 0x08U;
constexpr UINTPTR kStatusOffset = 0x0CU;
constexpr u32 kIrqPendingMask = 0x00000001U;
constexpr u32 kIrqAcknowledgeMask = 0x00000001U;

} // namespace

TemperatureDriver::TemperatureDriver()
    : base_address_(0U),
      interrupt_sequence_(0U)
{
}

TemperatureDriver::Status TemperatureDriver::initialize(
    UINTPTR base_address)
{
    if (base_address == static_cast<UINTPTR>(0U))
    {
        return Status::InvalidArgument;
    }

    base_address_ = base_address;
    interrupt_sequence_ = 0U;

    /* Remove a stale pending interrupt before connecting the GIC. */
    acknowledgeInterrupt();

    return Status::Success;
}

TemperatureDriver::Status TemperatureDriver::configureInterrupt(
    XScuGic &interrupt_controller,
    u32 interrupt_id,
    u8 priority,
    u8 trigger_type)
{
    XScuGic_SetPriorityTriggerType(
        &interrupt_controller,
        interrupt_id,
        priority,
        trigger_type);

    const s32 status = XScuGic_Connect(
        &interrupt_controller,
        interrupt_id,
        &TemperatureDriver::interruptHandler,
        this);

    if (status != XST_SUCCESS)
    {
        return Status::InterruptConnectError;
    }

    acknowledgeInterrupt();
    XScuGic_Enable(&interrupt_controller, interrupt_id);

    return Status::Success;
}

TemperatureDriver::Status TemperatureDriver::readSample(
    Sample &sample) const
{
    if (base_address_ == static_cast<UINTPTR>(0U))
    {
        return Status::InitializationError;
    }

    sample.output_scale = Xil_In32(base_address_ + kScaleOffset);
    sample.temperature_x10 =
        static_cast<s32>(Xil_In32(base_address_ + kTemperatureX10Offset));
    sample.raw_value = Xil_In32(base_address_ + kRawOffset);
    sample.status = Xil_In32(base_address_ + kStatusOffset);

    return Status::Success;
}

u32 TemperatureDriver::interruptSequence() const
{
    return interrupt_sequence_;
}

void TemperatureDriver::acknowledgeInterrupt() const
{
    if (base_address_ == static_cast<UINTPTR>(0U))
    {
        return;
    }

    Xil_Out32(
        base_address_ + kStatusOffset,
        kIrqAcknowledgeMask);

    /* Read back to ensure the AXI write completed before leaving the ISR. */
    static_cast<void>(
        Xil_In32(base_address_ + kStatusOffset) & kIrqPendingMask);
}

void TemperatureDriver::interruptHandler(void *callback_reference)
{
    TemperatureDriver *const driver =
        static_cast<TemperatureDriver *>(callback_reference);

    if (driver == nullptr)
    {
        return;
    }

    /* The source is level-held, so clear it before returning from the ISR. */
    driver->acknowledgeInterrupt();

    /* Main context performs all AXI reads, classification, GPIO and logging. */
    ++driver->interrupt_sequence_;
}

} // namespace app
