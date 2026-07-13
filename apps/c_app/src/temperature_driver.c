#include "temperature_driver.h"
#include <stddef.h>

#include "xil_io.h"
#include "xstatus.h"

#define TEMPERATURE_SCALE_OFFSET          0x00U
#define TEMPERATURE_X10_OFFSET            0x04U
#define TEMPERATURE_RAW_OFFSET            0x08U
#define TEMPERATURE_STATUS_OFFSET         0x0CU

#define TEMPERATURE_IRQ_PENDING_MASK      0x00000001U
#define TEMPERATURE_IRQ_ACKNOWLEDGE_MASK  0x00000001U

static void TemperatureDriver_InterruptHandler(void *callback_reference)
{
    TemperatureDriver *driver =
        (TemperatureDriver *)callback_reference;

    if (driver == NULL)
    {
        return;
    }

    /*
     * The mock IP uses a level triggered interrupt:
     *   status bit 0 = IRQ pending
     *   write bit 0 = "1" to clear IRQ
     *
     * Clear the hardware source first so the GIC does not immediately
     * re-enter this ISR.
     */
    TemperatureDriver_AcknowledgeInterrupt(driver);

    /*
     * Keep the ISR short. The main context performs AXI reads,
     * conversion, state evaluation, GPIO updates, and logging.
     */
    driver->interrupt_sequence++;
}

TemperatureDriverStatus TemperatureDriver_Initialize(
    TemperatureDriver *driver,
    UINTPTR base_address)
{
    if ((driver == NULL) || (base_address == (UINTPTR)0U))
    {
        return TEMPERATURE_DRIVER_INVALID_ARGUMENT;
    }

    driver->base_address = base_address;
    driver->interrupt_sequence = 0U;

    /* Remove a stale pending interrupt before connecting the GIC. */
    TemperatureDriver_AcknowledgeInterrupt(driver);

    return TEMPERATURE_DRIVER_SUCCESS;
}

TemperatureDriverStatus TemperatureDriver_ConfigureInterrupt(
    TemperatureDriver *driver,
    XScuGic *interrupt_controller,
    u32 interrupt_id,
    u8 priority,
    u8 trigger_type)
{
    s32 status;

    if ((driver == NULL) || (interrupt_controller == NULL))
    {
        return TEMPERATURE_DRIVER_INVALID_ARGUMENT;
    }

    XScuGic_SetPriorityTriggerType(
        interrupt_controller,
        interrupt_id,
        priority,
        trigger_type);

    status = XScuGic_Connect(
        interrupt_controller,
        interrupt_id,
        (Xil_InterruptHandler)TemperatureDriver_InterruptHandler,
        driver);

    if (status != XST_SUCCESS)
    {
        return TEMPERATURE_DRIVER_INTERRUPT_CONNECT_ERROR;
    }

    /*
     * Clear once more immediately before enabling, in case the IP generated
     * a sample while the interrupt controller was being configured.
     */
    TemperatureDriver_AcknowledgeInterrupt(driver);

    XScuGic_Enable(
        interrupt_controller,
        interrupt_id);

    return TEMPERATURE_DRIVER_SUCCESS;
}

TemperatureDriverStatus TemperatureDriver_ReadSample(
    const TemperatureDriver *driver,
    TemperatureSample *sample)
{
    if ((driver == NULL) || (sample == NULL))
    {
        return TEMPERATURE_DRIVER_INVALID_ARGUMENT;
    }

    sample->output_scale = Xil_In32(
        driver->base_address + TEMPERATURE_SCALE_OFFSET);

    sample->temperature_x10 = (s32)Xil_In32(
        driver->base_address + TEMPERATURE_X10_OFFSET);

    sample->raw_value = Xil_In32(
        driver->base_address + TEMPERATURE_RAW_OFFSET);

    sample->status = Xil_In32(
        driver->base_address + TEMPERATURE_STATUS_OFFSET);

    return TEMPERATURE_DRIVER_SUCCESS;
}

u32 TemperatureDriver_GetInterruptSequence(
    const TemperatureDriver *driver)
{
    if (driver == NULL)
    {
        return 0U;
    }

    return driver->interrupt_sequence;
}

void TemperatureDriver_AcknowledgeInterrupt(
    const TemperatureDriver *driver)
{
    if (driver == NULL)
    {
        return;
    }

    Xil_Out32(
        driver->base_address + TEMPERATURE_STATUS_OFFSET,
        TEMPERATURE_IRQ_ACKNOWLEDGE_MASK);

    // Read back to ensure the write has completed and the IRQ is cleared.
    (void)(Xil_In32(
        driver->base_address + TEMPERATURE_STATUS_OFFSET) &
        TEMPERATURE_IRQ_PENDING_MASK);
}
