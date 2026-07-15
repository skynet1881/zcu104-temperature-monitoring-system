## Temperature IP Register Map
## Overview
The temperature mock IP is a custom AXI4-lite peripheral. 
It generates simulated temperature values and exposes them to software through 32 bit registers.

Peripheral also generates interrupt whenever new temperature value is available.

## Register Map
Offset | Register Name | Access | Description  
0x00 | Scale | Read-only | Descrobes the raw output scale  
0x04 | Temperaturex10 | Read-only | Current temperature value multiplied by 10  
0x08 | Temperature-raw | Read-only | Current temperature value in raw format  
0x0C | Interrupt status | Read/Write | Interrupt status register. Writing 1 clears the interrupt pending flag.  