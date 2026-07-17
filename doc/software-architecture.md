## Software Architecture
## Overview
The software runs as a bare-metal application on the Cortex-R5 processor of the Zynq UltraScale+ MPSoC. 
The application reads simulated temperature value from custom temperature mock IP, reads system configuration data from EEPROM and controls four LEDs throgh AXI GPIO peripheral. 

```mermaid
graph TD
    main --> SystemManager
    SystemManager --> TemperatureMonitor
    SystemManager --> EEPROMManager
    SystemManager --> LEDController
```

### Main Module
main.c is a application entry point. Calling system_manager_init() initialized all system components and start main application loop.

### System Manager
The system manager is responsible for orchestrating the temperature monitoring system. It initializes all system components and start application from main. 
Responsibilities include:
- Initialize temperature monitor.
- Initialize EEPROM manager.
- Initialize LED controller.
- Initizalize interrupt controller.
- Compare temperature value.
- Report errors.

## GPIO Driver
GPIO driver encapsulates the AXI GPIO peripheral and controls LEDs.

## Temperature Driver
The temperature driver encapsulates custom temperature mock IP.
Responsibilities include:
- Read temperature value from temperature registers.
- Read interrupt status register.
- Acknowledge interrupt source.
- Handling temperature update interrupt.

## Runtime Flow
The runtime flow of the application is as follows:
1. The application starts and initializes the system manager.
2. The system manager initializes the temperature monitor, interrupt controller, EEPROM manager, and LED controller.
3. In each interrupt, sequence value increased and temperature value is read from temperature driver.
4. The system manager sets GPIO output based on temperature value. 
