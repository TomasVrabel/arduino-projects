This Arduino project controls an automated water pump system.

### Key Features:

*   **Flow Measurement:** Measures water flow in liters per minute, keeping track of historical data and total volume pumped.
*   **Pump Control:** Offers both manual and automatic pump operation.
*   **Automatic Shut-off:** The pump automatically turns off if the flow rate is too low, preventing damage.
*   **Modes of Operation:**
    *   **Measure Mode (0):** Only measures flow; pump control is disabled.
    *   **Manual Mode (1):** Measures flow and allows manual starting/stopping of the pump.
    *   **Auto Mode (2):** Measures flow, allows manual control, and automatically runs the pump at a configurable interval (default is every 2 hours).
*   **Bluetooth Interface:** The system is controlled via a Bluetooth serial terminal on a smartphone. Commands allow for:
    *   Viewing summaries (short and detailed).
    *   Starting and stopping the pump.
    *   Switching between modes.
    *   Adjusting the automatic pump interval.
    *   Resetting statistics stored in EEPROM.
*   **Data Persistence:** Uses EEPROM to store total volume and measurement counts, so data is not lost on restart.

### Hardware Components:

*   Arduino board
*   Bluetooth module (e.g., CC41-A)
*   Flowmeter
*   433MHz RF transmitter to control a remote power socket
*   DS3231 Real-Time Clock (RTC) module for timekeeping

The project is structured into multiple `.ino` files for better organization, separating the main loop, controller logic, memory utilities, and other helper functions. It appears to be in Slovak.

### Supported Flow Pulse Meters ###

#### YF-B10 ####

Specifications:
* Working Voltage: DC 5 - 15V
* Liquid Flow Rate: 2 - 50L/min
* Conversion Characteristic: F=(8 * Q)-4 +-5%, F - frequecny, Q - flow in liter/minute; 
 * also found: f = (6 x Q - 8)
 * also found F = 6.5163 x Q - 2.7 here: https://github.com/bway-dev/flow-sensor-esp32
* Maximum Current: 15 mA (DC 5V)
* Operating Temperature: -25 ~ +80°C
* Operating Humidity: 35% ~ 90% RH (without frost)
* Outer Thread: 1" DN25
* Body Length: 58 mm
* Thread Length: 10 mm
* Output: 5V TTL
* Cable Length: ~15 cm
* Wires: Red VCC, Yellow OUT, Black GND
* Connector: JST SM

#### YF-B6 ####

Specifications
* Working Voltage: DC 5 - 15V
* Liquid Flow Rate: 1 - 30L/min
* Maximum Current: 15 mA (DC 5V)
* Operating Temperature: -25 ~ +80°C
* Operating Humidity: 35% ~ 90% RH
* Outer Thread: 3/4"
* Output: 5V TTL
* Body Length: 60 mm
* Thread Length: 11 mm
* Pulse Characteristics: F = 6.6 * Q; 198 Hz (Q – 30 l/min), 99 Hz (Q – 15 l/min), 33 Hz (Q – 5 l/min)
* Cable Length: ~ 15 cm
* Body Material: Brass
* Connector: JST SM
* Wires: Red VCC, Yellow OUT, Black GND
