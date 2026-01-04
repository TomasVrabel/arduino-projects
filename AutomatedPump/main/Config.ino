#define CONFIG_TAG 123456100   // change magic number when doing breaking change in config
#define STORAGE_ADDRESS 128    // write on address 128, nano EEPROM size is 1024,  config size ~50bytes

void loadConfiguration() {
  EEPROM.get(STORAGE_ADDRESS, config);
  if (config.tag != CONFIG_TAG) {
    // Use default configuration
    config.tag = CONFIG_TAG;
    config.pulseFormula_coefficient = 8.0;
    config.pulseFormula_offset = 4.0;
    config.ThresholdMeasureStart = 5;       // measure when flow is above this value (in l/min)
    config.ThresholdMeasureStop = 2;        // stop measure when flow is below in this valie (in l/min)
    config.Mode = MODE_REPEAT;              // operation mode - just measure, manual or scheduled pump
    config.ThresholdTurnOff = 10;           // turn off when flow below this value (in l/min) 
    config.TresholdTurnOffSeconds = 3;      // turn off when flow is below ThresholdTurnOff for more this value (in seconds)
    config.DelayStartPump = 0;              // (in seconds) when pump start this delay interval will postpone evaluation of TresholdTurnOffSeconds. Idea behind: it takes time for pump to trigger flow, longer than 5 sec..
    config.RepeatIntervalMins = 120;        // interval (in mins) between scheduled pump actions
  }
}

void saveConfiguration() {
  EEPROM.put(STORAGE_ADDRESS, config);
}
