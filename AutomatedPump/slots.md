# Configuration Slots

This file documents the mapping of configuration slots to their corresponding parameters in the AutomatedPump system. You can use these slots to dynamically change the system's configuration via Bluetooth commands.

## How to Change Configuration

To change a configuration value, send a command in the following format:

`Set <slot> <value>`

- `<slot>`: The integer ID of the configuration parameter you want to change.
- `<value>`: The new value for the parameter.

For example, to set the `pulseFormula_coefficient` to `8.5`, you would send:

`Set 1 8.5`

## How to Save Configuration

To permanently save the changes you've made, send the following command:

`X`

This will write the current in-memory configuration to the EEPROM, so it will be loaded the next time the device starts up.

## Slot Mapping Table

| Slot | Parameter Name            | Data Type | Description                                                                 |
|------|---------------------------|-----------|-----------------------------------------------------------------------------|
| 1    | `pulseFormula_coefficient`| `float`   | The coefficient in the pulse formula `F = a * Q - b`.                         |
| 2    | `pulseFormula_offset`     | `float`   | The offset in the pulse formula `F = a * Q - b`.                              |
| 3    | `ThresholdMeasureStart`   | `byte`    | The flow rate (in l/min) above which to start measuring.                      |
| 4    | `ThresholdMeasureStop`    | `byte`    | The flow rate (in l/min) below which to stop measuring.                       |
| 5    | `Mode`                    | `byte`    | The operating mode of the pump (e.g., 0 for Measure, 1 for Manual, 2 for Repeat). |
| 6    | `ThresholdTurnOff`        | `byte`    | The flow rate (in l/min) below which to turn off the pump.                    |
| 7    | `TresholdTurnOffSeconds`  | `byte`    | The duration (in seconds) for which the flow must be below the threshold to turn off the pump. |
| 8    | `DelayStartPump`          | `long`    | The delay (in seconds) before the pump starts evaluating the turn-off threshold. |
| 9    | `RepeatIntervalMins`      | `uint32_t`| The interval (in minutes) at which the pump should repeat its operation.      |
| 100  | `nextScheduleMins`        | `HHmm`    | Sets the time for the next scheduled pump operation. The time is specified in 24-hour format, where HH represents hours and mm represents minutes. For example, to set the next schedule to 14:30, you would send `Set 100 1430`. This is a dynamic value and is not stored in EEPROM. |
