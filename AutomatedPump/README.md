# AutomatedPump
Software controls automatic pump.

Supported features:
* Flow measurement with history and total statistics
* Manual and automatic pump control
* Pump is automatically turned off if flow is very small

## User Interface

Primary interface is Bluetooth Serial Terminal installed on smart phone. 

Tested: Arduino BlueTooth Controller by ArgonDev, v2.14. Other tools tested by experiences random disconnects, unable to pair device etc.

Connect to device CC41-A and use following commands:
```
 S - Show detail summary
 s - Show short summary
 1 - Start pump (suported in mode 1 and 2)
 0 - Stop pump 
 m - Switch mode
 r/R - Increase/decrease pump interval in mode 2
 
 a - Show debug summary
 f/F - Increase/desrease syntetic flow (debug mode only) 
 X - Reset EEPROM memory with statistics, restart needed
```

 ## Modes
 * 0 - MEASURE MODE - only measure flow. Manual or automatic pump controls are turned off.
 * 1 - MANUAL MODE - measure flow and turn on/off pump manually. Automatic pump control is turned off
 * 2 - AUTO MODE - measure flow and support manual and automatic pump control. Pump is automatically turned on  every 2 hours, can be adjusted by r/R commands.


## Implementation

### Libraries
 * RCSwitch by sui77, version 2.6.4, installed using Arduino IDE 2.1.1 Library Manager
 * DS3231 - Download from https://navody.dratek.cz/docs/texty/0/243/arduino_ds3231_master.zip , libraries in Library Manager are not compatible

### Improvements
 * Slovak interface
 * fix: next pump should consider also dates, not only hours and min to handle day change at 12pm
 * decrease pump threshold to 1, make configurable
 * make configurable by BT
 * daily, previous day
 * total: pumps, measurements, volume
 * store in memory: today, last day, mode
 * remote control APP
 * solder, make more robust, isolate flowmeter cable
 * improve docus
 * LOW PRIO: mode: scheduled, schedule 
 * LOW PRIO: don't use Strings, other code and stability optimizations
 * LOW PRIO: replace DS3231 wit other librar
