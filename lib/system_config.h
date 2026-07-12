#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "xparameters.h"

/* Temperature mock IP: 0xA0010000 - 0xA001FFFF. */
#define SYSTEM_TEMPERATURE_BASE_ADDRESS \
    XPAR_TEMPERATURE_MOCK_0_S_AXI_BASEADDR

#define SYSTEM_TEMPERATURE_HIGH_ADDRESS \
    XPAR_TEMPERATURE_MOCK_0_S_AXI_HIGHADDR

/* Temperature interrupt: RPU GIC interrupt ID 121. */
#define SYSTEM_TEMPERATURE_INTERRUPT_ID \
    XPAR_FABRIC_TEMPERATURE_MOCK_0_IRQ_INTR

#define SYSTEM_INTERRUPT_CONTROLLER_DEVICE_ID \
    XPAR_SCUGIC_0_DEVICE_ID

/* AXI GPIO: 0xA0000000 - 0xA000FFFF, device ID 0. */
#define SYSTEM_GPIO_DEVICE_ID \
    XPAR_AXI_GPIO_0_DEVICE_ID

#define SYSTEM_GPIO_LED_CHANNEL 1U

/*
 * LED mapping:
 * bit 0 = green
 * bit 1 = yellow
 * bit 2 = red
 */
#define SYSTEM_GPIO_GREEN_LED_MASK  0x01U
#define SYSTEM_GPIO_YELLOW_LED_MASK 0x02U
#define SYSTEM_GPIO_RED_LED_MASK    0x04U
#define SYSTEM_GPIO_LED_MASK        0x07U

/* PS I2C 1 is exposed as XIicPs instance 0. */
#define SYSTEM_EEPROM_I2C_DEVICE_ID \
    XPAR_XIICPS_0_DEVICE_ID

#define SYSTEM_EEPROM_I2C_ADDRESS      0x50U
#define SYSTEM_EEPROM_I2C_CLOCK_HZ     100000U
#define SYSTEM_EEPROM_ADDRESS_BYTES    2U

/*
 * EEPROM configuration layout used by this application.
 *
 * 0x0000: hardware revision, one byte
 *         0 = Rev-A
 *         1 = Rev-B
 *
 * 0x0001..0x0007: seven ASCII serial-number bytes, for example ABC1234
 */
#define SYSTEM_EEPROM_REVISION_ADDRESS      0x0000U
#define SYSTEM_EEPROM_SERIAL_ADDRESS        0x0001U
#define SYSTEM_EEPROM_SERIAL_LENGTH         7U

/*
 * The mock IP produces a new sample every 100 us. Polling is used in this
 * version. The interrupt identifiers above are kept for the interrupt-driven
 * version.
 */
#define SYSTEM_MANAGER_POLL_INTERVAL_US     100U

#endif /* SYSTEM_CONFIG_H */
