# zcu104-temperature-monitoring-system
ZCU104 temperature monitoring system - this version treats the EEPROM as read only configuration storage.  
System reads EEPROM and decides which hardware version is being used.  

## Runtim flow

1. Init GPIO
2. Init EEPROM interface
3. Read hardware version from EEPROM
4. Init temperature sensor interface
5. Read temperature sensor data
6. Convert raw sample to temperature value
7. Drive LEDs

## EEPROM layout

| Address | Size | Meaning |
|---:|---:|---|
| `0x0000` | 1 byte | Hardware revision: `0` = Rev-A, `1` = Rev-B |
| `0x0001` | 7 bytes | ASCII serial number, for example `ABC1234` |

Change these addresses in `include/system_config.h` if the programmed EEPROM
layout is different.

## Temperature conversion

The internal software representation is `temperature_x10`.

- Rev-A: one raw digit equals 1 degree Celsius.
  - `temperature_x10 = raw_value * 10`
- Rev-B: one raw digit equals 0.1 degree Celsius.
  - `temperature_x10 = raw_value`

Examples:

- Rev-A raw `10` becomes `100`, meaning 10.0 C.
- Rev-B raw `100` remains `100`, meaning 10.0 C.

## LED thresholds

| Condition | LED |
|---|---|
| Temperature `< 5.0 C` | Red |
| Temperature `>= 105.0 C` | Red |
| Temperature `>= 85.0 C` and `< 105.0 C` | Yellow |
| Otherwise | Green |

The default AXI GPIO mapping is: (in this revision all LEDs are green)

- Bit 0: green
- Bit 1: yellow
- Bit 2: red

## Hardware identifiers

The current BSP provides:

| Peripheral | Identifier | Value |
|---|---|---:|
| Temperature mock base | `XPAR_TEMPERATURE_MOCK_0_S_AXI_BASEADDR` | `0xA0010000` |
| AXI GPIO base | `XPAR_AXI_GPIO_0_BASEADDR` | `0xA0000000` |
| AXI GPIO device ID | `XPAR_AXI_GPIO_0_DEVICE_ID` | `0` |
| PS I2C / XIicPs | `XPAR_XIICPS_0_DEVICE_ID` | `0` |
| Temperature interrupt | `XPAR_FABRIC_TEMPERATURE_MOCK_0_IRQ_INTR` | `121` |

## Note about the 100 us requirement

This package currently uses polling to keep the implementation simple. For
very low jitter, the next version should use the already connected temperature
interrupt and let the system manager process a sample-ready flag set by the ISR.