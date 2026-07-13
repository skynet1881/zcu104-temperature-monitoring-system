#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "xparameters.h"

/*
 * These names match the xparameters.h generated for the current
 * ZCU104 Cortex-R5 platform.
 */
#define APP_TEMPERATURE_BASE_ADDRESS      XPAR_TEMPERATURE_MOCK_0_S_AXI_BASEADDR
#define APP_TEMPERATURE_INTERRUPT_ID      XPAR_FABRIC_TEMPERATURE_MOCK_0_IRQ_INTR
#define APP_GIC_DEVICE_ID                 XPAR_SCUGIC_0_DEVICE_ID
#define APP_GPIO_DEVICE_ID                XPAR_AXI_GPIO_0_DEVICE_ID
#define APP_I2C_DEVICE_ID                 XPAR_XIICPS_0_DEVICE_ID

/* ZCU104 board EEPROM connection used by the current design. */
#define APP_EEPROM_I2C_ADDRESS            0x50U
#define APP_EEPROM_I2C_CLOCK_HZ           100000U

/*
 * Application-specific EEPROM layout.
 *
 * Byte 0    : hardware revision: 0 = Rev-A, 1 = Rev-B
 * Bytes 1-7 : seven-character serial number, for example "ABC1234"
 *
 * Adjust these offsets if your EEPROM image uses another layout.
 */
#define APP_EEPROM_REVISION_OFFSET        0x0000U
#define APP_EEPROM_SERIAL_OFFSET          0x0001U

/* AXI GPIO channel and LED bit mapping. */
#define APP_GPIO_CHANNEL                  1U
#define APP_LED_GREEN_MASK                0x00000001U
#define APP_LED_YELLOW_MASK               0x00000002U
#define APP_LED_RED_MASK                  0x00000004U
#define APP_LED_ALL_MASK                  \
    (APP_LED_GREEN_MASK | APP_LED_YELLOW_MASK | APP_LED_RED_MASK)

/*
 * XScuGic trigger encoding:
 *   0x01 = level-sensitive
 *   0x03 = rising-edge sensitive
 *
 * The current temperature IP holds IRQ high until software writes 1 to
 * register 0x0C. Therefore, level-sensitive configuration is required.
 */
#define APP_TEMPERATURE_IRQ_PRIORITY      0xA0U
#define APP_TEMPERATURE_IRQ_TRIGGER_TYPE  0x01U

/* Temperature limits expressed in tenths of one degree Celsius. */
#define APP_CRITICAL_LOW_X10              50
#define APP_WARNING_X10                   850
#define APP_CRITICAL_HIGH_X10             1050

#endif
