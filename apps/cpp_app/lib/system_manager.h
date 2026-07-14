#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include "eeprom_driver.h"
#include "gpio_driver.h"
#include "system_types.h"
#include "temperature_driver.h"
#include "xscugic.h"
#include "xil_types.h"

namespace app
{

class SystemManager
{
public:
    enum class Status : u8
    {
        Success = 0U,
        InvalidState,
        GpioInitializationError,
        EepromInitializationError,
        ConfigurationReadError,
        TemperatureInitializationError,
        GicInitializationError,
        InterruptInitializationError,
        SensorScaleMismatch,
        TemperatureReadError,
        GpioUpdateError
    };

    SystemManager();

    SystemManager(const SystemManager &) = delete;
    SystemManager &operator=(const SystemManager &) = delete;

    Status initialize();
    Status process();

private:
    Status initializeInterruptController();
    Status processTemperatureSample(bool verify_scale);
    void setFallbackConfiguration();

    static u32 expectedScale(HardwareRevision revision);
    static s32 convertRawToX10(u32 raw_value, HardwareRevision revision);
    static TemperatureState determineTemperatureState(s32 temperature_x10);
    static const char *stateName(TemperatureState state);
    static void printTemperature(s32 temperature_x10);

    GpioDriver gpio_;
    EepromDriver eeprom_;
    TemperatureDriver temperature_;
    XScuGic interrupt_controller_;

    SystemConfiguration configuration_;
    TemperatureState temperature_state_;
    u32 last_processed_interrupt_sequence_;
    bool initialized_;
};

} // namespace app

#endif
