#include "eeprom_driver.hpp"

#include "system_config.hpp"
#include "xstatus.h"

namespace app
{
namespace
{

constexpr u32 kAddressSizeBytes = 2U;
constexpr u32 kBusTimeoutLoops = 1000000U;

} // namespace

EepromDriver::EepromDriver()
    : instance_{},
      slave_address_(0U),
      initialized_(false)
{
}

EepromDriver::Status EepromDriver::initialize(
    u16 device_id,
    u16 slave_address,
    u32 serial_clock_hz)
{
    XIicPs_Config *const configuration = XIicPs_LookupConfig(device_id);
    if (configuration == nullptr)
    {
        return Status::LookupError;
    }

    const s32 initialize_status = XIicPs_CfgInitialize(
        &instance_,
        configuration,
        configuration->BaseAddress);

    if (initialize_status != XST_SUCCESS)
    {
        return Status::InitializationError;
    }

    const s32 clock_status = XIicPs_SetSClk(
        &instance_,
        serial_clock_hz);

    if (clock_status != XST_SUCCESS)
    {
        return Status::ClockError;
    }

    slave_address_ = slave_address;
    initialized_ = true;

    return Status::Success;
}

EepromDriver::Status EepromDriver::waitUntilBusIdle()
{
    u32 timeout = kBusTimeoutLoops;

    while (XIicPs_BusIsBusy(&instance_) != 0)
    {
        if (timeout == 0U)
        {
            return Status::BusBusyTimeout;
        }

        --timeout;
    }

    return Status::Success;
}

EepromDriver::Status EepromDriver::readBytes(
    u16 memory_address,
    u8 *data,
    u32 length)
{
    if ((!initialized_) || (data == nullptr) || (length == 0U))
    {
        return Status::InvalidArgument;
    }

    Status status = waitUntilBusIdle();
    if (status != Status::Success)
    {
        return status;
    }

    u8 address_bytes[kAddressSizeBytes];
    address_bytes[0] = static_cast<u8>((memory_address >> 8U) & 0xFFU);
    address_bytes[1] = static_cast<u8>(memory_address & 0xFFU);

    XIicPs_SetOptions(&instance_, XIICPS_REP_START_OPTION);

    const s32 send_status = XIicPs_MasterSendPolled(
        &instance_,
        address_bytes,
        kAddressSizeBytes,
        slave_address_);

    if (send_status != XST_SUCCESS)
    {
        XIicPs_ClearOptions(&instance_, XIICPS_REP_START_OPTION);
        return Status::AddressSendError;
    }

    const s32 receive_status = XIicPs_MasterRecvPolled(
        &instance_,
        data,
        length,
        slave_address_);

    XIicPs_ClearOptions(&instance_, XIICPS_REP_START_OPTION);

    if (receive_status != XST_SUCCESS)
    {
        return Status::ReadError;
    }

    return waitUntilBusIdle();
}

char EepromDriver::sanitizeSerialCharacter(u8 value)
{
    if ((value >= static_cast<u8>(' ')) &&
        (value <= static_cast<u8>('~')))
    {
        return static_cast<char>(value);
    }

    return '?';
}

EepromDriver::Status EepromDriver::readConfiguration(
    SystemConfiguration &configuration)
{
    u8 revision = 0U;
    Status status = readBytes(
        config::kEepromRevisionOffset,
        &revision,
        1U);

    if (status != Status::Success)
    {
        return status;
    }

    if (revision == static_cast<u8>(HardwareRevision::RevisionA))
    {
        configuration.hardware_revision = HardwareRevision::RevisionA;
    }
    else if (revision == static_cast<u8>(HardwareRevision::RevisionB))
    {
        configuration.hardware_revision = HardwareRevision::RevisionB;
    }
    else
    {
        return Status::InvalidConfiguration;
    }

    u8 serial[kSystemSerialNumberLength];
    status = readBytes(
        config::kEepromSerialOffset,
        serial,
        kSystemSerialNumberLength);

    if (status != Status::Success)
    {
        return status;
    }

    for (u32 index = 0U; index < kSystemSerialNumberLength; ++index)
    {
        configuration.serial_number[index] =
            sanitizeSerialCharacter(serial[index]);
    }

    configuration.serial_number[kSystemSerialNumberLength] = '\0';
    return Status::Success;
}

} // namespace app
