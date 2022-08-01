
#define MODE_MEASURE 0
#define MODE_MANUAL 1
#define MODE_REPEAT 2
#define MODE_SCHEDULE 3

#define MODES_COUNT 4

#define DAY_MINUTES 1440

byte Mode = 0;

byte ThresholdMeasureStart = 2; // measure when flow is above 1 l/min
byte ThresholdMeasureStop = 1; // stop measure when flow is below 1 l/min

byte ThresholdTurnOff = 3; // turn off when flow below 3 l/min  
byte TresholdTurnOffSeconds = 5; // turn off whjen flow is below ThresholdTurnOff for more then 5 seconds

byte RepeatIntervalMins = 60;

unsigned long aboveThresholdMeasureTs = 0;
unsigned long aboveThresholdTurnOffTs = 0;

unsigned int nextScheduleMins;
bool measuring = false;
bool pumping = false;

unsigned int currentMeasurementMl = 0;
unsigned int currentMeasurementMin = 0;

void ProcessFlowData() {
  long ts = millis();
  if (flow > ThresholdMeasureStop)  aboveThresholdMeasureTs = ts;
  if (flow > ThresholdTurnOff)  aboveThresholdTurnOffTs = ts;

  if (pumping && Mode > MODE_MEASURE && (ts - aboveThresholdTurnOffTs) > TresholdTurnOffSeconds * 1000) { 
    BT_println(F("Low flow while pumping, stopping pump."));
    StopPump();
  }

  if (!pumping) {
    CheckSchedule();
  }
  
  if (measuring && ThresholdMeasureStop > flow) { 
    // finish measurement
    BT_println(F("Stop measuring"));
    measuring = false;

    LCD_updateLastMeasurement(currentMeasurementMin, GetCurrentMins() - currentMeasurementMin,  currentMeasurementMl/1000);
    
  } else if (!measuring && ThresholdMeasureStart < flow)  {
    // start measure
    BT_print(F("Stop measuring, current: ")); BT_println(String(currentMeasurementMl/1000) + " l");
    
    measuring = true;
    currentMeasurementMl = 0;
    currentMeasurementMin = GetCurrentMins();
  }

  if (measuring) {
    currentMeasurementMl += flowML;
  }

  LCD_updateCurrentMeasurement(measuring, currentMeasurementMin, GetCurrentMins() - currentMeasurementMin,  currentMeasurementMl/1000);
}

void InitController() {
  SetMode(MODE_MEASURE);
}

void SetMode(byte mode) {
  BT_println("SetMode, old : " + String(Mode));
  
  Mode = mode % MODES_COUNT;
  
  BT_println("New mode: " + String(Mode));
  
  ScheduleNextIteration();
  LCD_updateMode();
}

void SwitchMode() {
  SetMode(Mode+1);
}

void ScheduleNextIteration() {
  if (Mode == MODE_MEASURE || Mode == MODE_MANUAL) return;
  if (Mode == MODE_REPEAT) {
     nextScheduleMins = (GetCurrentMins() + RepeatIntervalMins) % DAY_MINUTES;
  }
}

void StartPump() {
  BT_println(F("Start pumping"));
   
  Socket_On(); 
  pumping = true;
  aboveThresholdTurnOffTs = millis();
}

void StopPump() {
  BT_println(F("Stop pumping"));
  
  Socket_Off();

  if (pumping) {
    ScheduleNextIteration();
  }
  
  pumping = false;
}

unsigned int GetCurrentMins() {
  return (dateTime.hour * 60 + dateTime.minute) % DAY_MINUTES;
}

void CheckSchedule() {
  if( Mode >= MODE_REPEAT && nextScheduleMins < GetCurrentMins()) {
    StartPump();
  } 
}

void Manual_Off() {
  StopPump();
}

void Manual_On() {
  if (Mode == MODE_MEASURE) {
    BT_println(F("Cannot turn socket ON in MEASURE mode."));
    return;
  }
  StartPump();
}

void SendStatusBlueTooth() {
  // TBD: lot of stuff to send on blueetooth
  BT_print(F("Time since startup: ")); BT_print(String(millis()/1000)); BT_print(F(" secs."));
  BT_print(F("Date: ")); BT_println(FormatDate(&dateTime, true));
  BT_print(F("Time: ")); BT_println(FormatTime(&dateTime, true));
  
  BT_print(F("Mode: ")); BT_println(String(Mode));
  BT_print(F("Next schedule: ")); BT_println(String(nextScheduleMins / 60) + ":" + String(nextScheduleMins % 60));

  BT_print(F("Flow: ")); BT_println(String(flow) + " l/min");
  BT_print(F("Current pulses: ")); BT_println(String(pulseCount));
  BT_print(F("pumping: ")); BT_println(String(pumping));
  BT_print(F("measuring: ")); BT_println(String(measuring));
  
  BT_print(F("currentMeasurement.volume: ")); BT_println(String(currentMeasurementMl/1000) + " l");
  BT_print(F("currentMeasurement.start: ")); BT_println(String(currentMeasurementMin / 60) + ":" + String(currentMeasurementMin % 60));
  
  BT_print(F("ThresholdMeasureStart: ")); BT_println(String(ThresholdMeasureStart));
  BT_print(F("ThresholdMeasureStop: ")); BT_println(String(ThresholdMeasureStop));
  BT_print(F("ThresholdTurnOff: ")); BT_println(String(ThresholdTurnOff));
  BT_print(F("TresholdTurnOffSeconds: ")); BT_println(String(TresholdTurnOffSeconds));
  BT_print(F("RepeatIntervalMins: ")); BT_println(String(RepeatIntervalMins));
  
  BT_print(F("aboveThresholdMeasureTs: ")); BT_println(String(aboveThresholdMeasureTs/1000));
  BT_print(F("aboveThresholdTurnOffTs: ")); BT_println(String(aboveThresholdTurnOffTs/1000));
}
