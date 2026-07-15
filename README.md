# zcu104-temperature-monitoring-system
ZCU104 temperature monitoring system - this version treats the EEPROM as read only configuration storage.  
System reads EEPROM and decides which hardware version is being used.  

## How to create workspace and build the project
From xilinx tools repository, run following scripts in order to create workspace and build the project.
```bash
https://github.com/skynet1881/xilinx-dev-tools
```

Example usage for scripts:
```bash
chmod +x scripts/setup.sh
chmod +x scripts/build.sh

# setup script usage
./scripts/setup.sh <path_to_xsa> <path_to_workspace> <path_to_application_src <path_to_application_include>  

# exp: setup workspace and build
./scripts/setup.sh \
    xsa/design_1_wrapper.xsa \
    build/vitis_workspace \
    application/src \
    application/include

# build workspace
./scripts/build.sh build/vitis_workspace
```

## Runtime flow

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

## Documentation

- [System Architecture](docs/architecture.md)
- [Hardware Architecture](docs/hardware-architecture.md)
- [Software Architecture](docs/software-architecture.md)
- [Interrupt Handling](docs/interrupt-handling.md)
- [Temperature IP Register Map](docs/register-map.md)