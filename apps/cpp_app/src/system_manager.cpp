#include "system_manager.hpp"

#include "system_config.hpp"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xstatus.h"

namespace app
{

SystemManager::SystemManager()
    : gpio_{},
      eeprom_{},
      temperature_{},
      interrupt_controller_{},
      configuration_{HardwareRevision::RevisionA, {0}},
      temperature_state_(TemperatureState::Unknown),
      last_processed_interrupt_sequence_(0U),
      initialized_(false)
{
}

SystemManager::Status SystemManager::initializeInterruptController()
{
    XScuGic_Config *const configuration =
        XScuGic_LookupConfig(config::kGicDeviceId);

    if (configuration == nullptr)
    {
        return Status::GicInitializationError;
    }

    const s32 status = XScuGic_CfgInitialize(
        &interrupt_controller_,
        configuration,
        configuration->CpuBaseAddress);

    if (status != XST_SUCCESS)
    {
        return Status::GicInitializationError;
    }

    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(
        XIL_EXCEPTION_ID_INT,
        reinterpret_cast<Xil_ExceptionHandler>(XScuGic_InterruptHandler),
        &interrupt_controller_);

    return Status::Success;
}

u32 SystemManager::expectedScale(HardwareRevision revision)
{
    return (revision == HardwareRevision::RevisionA) ? 1U : 10U;
}

s32 SystemManager::convertRawToX10(
    u32 raw_value,
    HardwareRevision revision)
{
    if (revision == HardwareRevision::RevisionA)
    {
        return static_cast<s32>(raw_value * 10U);
    }

    return static_cast<s32>(raw_value);
}

TemperatureState SystemManager::determineTemperatureState(
    s32 temperature_x10)
{
    if ((temperature_x10 < config::kCriticalLowX10) ||
        (temperature_x10 >= config::kCriticalHighX10))
    {
        return TemperatureState::Critical;
    }

    if (temperature_x10 >= config::kWarningX10)
    {
        return TemperatureState::Warning;
    }

    return TemperatureState::Normal;
}

const char *SystemManager::stateName(TemperatureState state)
{
    switch (state)
    {
        case TemperatureState::Normal:
            return "NORMAL";

        case TemperatureState::Warning:
            return "WARNING";

        case TemperatureState::Critical:
            return "CRITICAL";

        default:
            return "UNKNOWN";
    }
}

void SystemManager::printTemperature(s32 temperature_x10)
{
    if (temperature_x10 < 0)
    {
        const s32 absolute_temperature = -temperature_x10;
        xil_printf(
            "-%d.%d",
            static_cast<int>(absolute_temperature / 10),
            static_cast<int>(absolute_temperature % 10));
        return;
    }

    xil_printf(
        "%d.%d",
        static_cast<int>(temperature_x10 / 10),
        static_cast<int>(temperature_x10 % 10));
}

void SystemManager::setFallbackConfiguration()
{
    configuration_.hardware_revision = HardwareRevision::RevisionA;

    static const char kFallbackSerial[] = "UNKNOWN";
    for (u32 index = 0U; index < kSystemSerialNumberLength; ++index)
    {
        configuration_.serial_number[index] = kFallbackSerial[index];
    }
    configuration_.serial_number[kSystemSerialNumberLength] = '\0';
}

SystemManager::Status SystemManager::processTemperatureSample(
    bool verify_scale)
{
    TemperatureDriver::Sample sample{};
    const TemperatureDriver::Status temperature_status =
        temperature_.readSample(sample);

    if (temperature_status != TemperatureDriver::Status::Success)
    {
        return Status::TemperatureReadError;
    }

    const u32 required_scale =
        expectedScale(configuration_.hardware_revision);

    if (verify_scale && (sample.output_scale != required_scale))
    {
        xil_printf(
            "ERROR: sensor scale mismatch: EEPROM expects %lu, IP reports %lu\r\n",
            static_cast<unsigned long>(required_scale),
            static_cast<unsigned long>(sample.output_scale));

        return Status::SensorScaleMismatch;
    }

    /*
     * Rev-A raw 10  -> 10.0 C -> x10 = 100
     * Rev-B raw 100 -> 10.0 C -> x10 = 100
     */
    const s32 converted_temperature_x10 = convertRawToX10(
        sample.raw_value,
        configuration_.hardware_revision);

    const TemperatureState new_state =
        determineTemperatureState(converted_temperature_x10);

    if (new_state != temperature_state_)
    {
        const GpioDriver::Status gpio_status =
            gpio_.showTemperatureState(new_state);

        if (gpio_status != GpioDriver::Status::Success)
        {
            return Status::GpioUpdateError;
        }

        xil_printf(
            "State change: %s -> %s, temperature=",
            stateName(temperature_state_),
            stateName(new_state));

        printTemperature(converted_temperature_x10);

        xil_printf(
            " C, raw=%lu, irq_sequence=%lu\r\n",
            static_cast<unsigned long>(sample.raw_value),
            static_cast<unsigned long>(temperature_.interruptSequence()));

        temperature_state_ = new_state;
    }

    return Status::Success;
}

SystemManager::Status SystemManager::initialize()
{
    initialized_ = false;
    temperature_state_ = TemperatureState::Unknown;
    last_processed_interrupt_sequence_ = 0U;

    const GpioDriver::Status gpio_status = gpio_.initialize(
        config::kGpioDeviceId,
        config::kGpioChannel);

    if (gpio_status != GpioDriver::Status::Success)
    {
        xil_printf(
            "ERROR: GPIO initialization failed: %d\r\n",
            static_cast<int>(gpio_status));
        return Status::GpioInitializationError;
    }

    const EepromDriver::Status eeprom_initialize_status =
        eeprom_.initialize(
            config::kI2cDeviceId,
            config::kEepromI2cAddress,
            config::kEepromI2cClockHz);

    if (eeprom_initialize_status != EepromDriver::Status::Success)
    {
        xil_printf(
            "ERROR: EEPROM initialization failed: %d\r\n",
            static_cast<int>(eeprom_initialize_status));
        gpio_.showError();
        return Status::EepromInitializationError;
    }

    const EepromDriver::Status eeprom_read_status =
        eeprom_.readConfiguration(configuration_);

    if (eeprom_read_status != EepromDriver::Status::Success)
    {
        xil_printf(
            "ERROR: EEPROM configuration read failed: %d\r\n",
            static_cast<int>(eeprom_read_status));

        if (!config::kAllowEepromFallback)
        {
            gpio_.showError();
            return Status::ConfigurationReadError;
        }

        setFallbackConfiguration();
        xil_printf("Using fallback configuration: Rev-A, serial=UNKNOWN\r\n");
    }

    xil_printf(
        "Configuration: revision=%d, serial=%s\r\n",
        static_cast<int>(configuration_.hardware_revision),
        configuration_.serial_number);

    const TemperatureDriver::Status temperature_initialize_status =
        temperature_.initialize(config::kTemperatureBaseAddress);

    if (temperature_initialize_status != TemperatureDriver::Status::Success)
    {
        xil_printf(
            "ERROR: temperature driver initialization failed: %d\r\n",
            static_cast<int>(temperature_initialize_status));
        gpio_.showError();
        return Status::TemperatureInitializationError;
    }

    Status status = initializeInterruptController();
    if (status != Status::Success)
    {
        xil_printf("ERROR: GIC initialization failed\r\n");
        gpio_.showError();
        return status;
    }

    const TemperatureDriver::Status interrupt_status =
        temperature_.configureInterrupt(
            interrupt_controller_,
            config::kTemperatureInterruptId,
            config::kTemperatureIrqPriority,
            config::kTemperatureIrqTriggerType);

    if (interrupt_status != TemperatureDriver::Status::Success)
    {
        xil_printf(
            "ERROR: temperature IRQ initialization failed: %d\r\n",
            static_cast<int>(interrupt_status));
        gpio_.showError();
        return Status::InterruptInitializationError;
    }

    /* Enable CPU IRQ exceptions only after the complete interrupt path exists. */
    Xil_ExceptionEnable();

    status = processTemperatureSample(true);
    if (status != Status::Success)
    {
        gpio_.showError();
        return status;
    }

    last_processed_interrupt_sequence_ = temperature_.interruptSequence();
    initialized_ = true;

    xil_printf(
        "Temperature monitoring started: IRQ ID=%lu, level-sensitive\r\n",
        static_cast<unsigned long>(config::kTemperatureInterruptId));

    return Status::Success;
}

SystemManager::Status SystemManager::process()
{
    if (!initialized_)
    {
        return Status::InvalidState;
    }

    const u32 current_interrupt_sequence = temperature_.interruptSequence();

    if (current_interrupt_sequence == last_processed_interrupt_sequence_)
    {
        return Status::Success;
    }

    last_processed_interrupt_sequence_ = current_interrupt_sequence;
    return processTemperatureSample(false);
}

} // namespace app
