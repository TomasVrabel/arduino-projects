
#define MODE_MEASURE 0
#define MODE_MANUAL 1
#define MODE_REPEAT 2

#define MODES_COUNT 3

#define DAY_MINUTES 1440
#define TIMEZONE_COMPENSATION 60  // library has bug/feature, unixtime is in UTC so we need to add 1 hour

byte Mode = 0;

byte ThresholdMeasureStart = 2; // measure when flow is above 1 l/min
byte ThresholdMeasureStop = 1; // stop measure when flow is below 1 l/min

byte ThresholdTurnOff = 1; // turn off when flow below 3 l/min  
byte TresholdTurnOffSeconds = 5; // turn off whjen flow is below ThresholdTurnOff for more then 5 seconds
byte DelayStartPump = 5;  // (secs) when pump start this delay interval will postpone evaluation of TresholdTurnOffSeconds. Idea behind: it takes time for pump to trigger flow, longer than 5 sec..

uint32_t RepeatIntervalMins = 60;

long aboveThresholdMeasureTs = 0;
long aboveThresholdTurnOffTs = 0;

uint32_t nextScheduleMins = 0;
bool measuring = false;
bool pumping = false;

unsigned int currentMeasurementMl = 0;
uint32_t currentMeasurementMin = 0;

void ProcessFlowData() {
  long ts = millis();
  if (flow > ThresholdMeasureStop)  aboveThresholdMeasureTs = ts;
  if (flow > ThresholdTurnOff)  aboveThresholdTurnOffTs = ts;

  if (pumping && Mode > MODE_MEASURE && (ts - aboveThresholdTurnOffTs) > (long) TresholdTurnOffSeconds * 1000) { 
    bt.println(F("Nizky prietok pri pustenom cerpadle, zastavujem cerpadlo!"));
    StopPump();
  }

  if (!pumping) {
    CheckSchedule();
  }
  
  if (measuring && ThresholdMeasureStop > flow) { 
    // finish measurement
    bt.println(F("Meranie zastavene, nizky prietok."));
    measuring = false;
 
  } else if (!measuring && ThresholdMeasureStart < flow)  {
    // start measure
    bt.println(F("Zacinam meranie, zisteni vyssi prietok."));
    
    measuring = true;
    currentMeasurementMl = 0;
    currentMeasurementMin = GetCurrentMins();
  }

  if (measuring) {
    currentMeasurementMl += flowML;
  }
}

void InitController() {
  SetMode(MODE_REPEAT);
}

void SetMode(byte mode) {
  Mode = mode % MODES_COUNT;
  
  bt.print(F("Zmena rezimu, novy rezim: ")); bt.println(Mode);
 
  ScheduleNextIteration();
}

void SwitchMode() {
  SetMode(Mode+1);
}

void ScheduleNextIteration() {
  if (Mode == MODE_MEASURE || Mode == MODE_MANUAL) return;
  if (Mode == MODE_REPEAT) {
    
    // we are scheduling next pump based on last schedule BUT if last schedule is fat in past we will schedule from current time
    if ( (nextScheduleMins + RepeatIntervalMins) < GetCurrentMins()) {
      nextScheduleMins = GetCurrentMins();
    }
      
    nextScheduleMins = nextScheduleMins + RepeatIntervalMins;
  }
  SendStatusBlueTooth();
}

void StartPump() {
  bt.println(F("Spustam cerpadlo."));
   
  Socket_On(); 
  pumping = true;
  aboveThresholdTurnOffTs = millis() + DelayStartPump * 1000;
}

void StopPump() {
  bt.println(F("Zastavujem cerpadlo."));
  
  Socket_Off();

  if (pumping) {
    ScheduleNextIteration(); 
  }
  
  pumping = false;
}

uint32_t GetCurrentMins() {
  return (dateTime.unixtime / 60) + TIMEZONE_COMPENSATION;
}

void CheckSchedule() {
  if( Mode == MODE_REPEAT && nextScheduleMins <= GetCurrentMins()) {
    bt.print("Nastal planovany cas cerpania: "); BT_printNextSchedule(nextScheduleMins); bt.println();
    StartPump();
  } 
}

void Manual_Off() {
  StopPump();
}

void Manual_On() {
  if (Mode == MODE_MEASURE) {
    bt.println(F("Spustit cerpadlo v rezime MERANIE nie je povolene"));
    return;
  }
  StartPump();
}

void UpdateRepeatInterval(int delta) {
  uint32_t p = RepeatIntervalMins + delta;
  if (p > 0 && p < 600)
    RepeatIntervalMins = p;
  bt.print(F("Novy interval spustania cerpadla (min): "));
  bt.println(RepeatIntervalMins);
}

void SendStatusBlueTooth() {
  // TBD: lot of stuff to send on blueetooth
  bt.print(F("Time since startup: ")); bt.print(millis()/1000); bt.println(F(" secs."));
  bt.print(F("Date: ")); BT_printDate(dateTime); bt.println();
  bt.print(F("Time: ")); BT_printTime(dateTime); bt.println();
  bt.print(F("GetCurrentMins: ")); bt.println(GetCurrentMins());
  bt.print(F("GetCurrentMins (formatted): ")); BT_printNextSchedule(GetCurrentMins()); bt.println();
    
  bt.print(F("Mode: ")); bt.println(Mode);
  bt.print(F("Next schedule: ")); BT_printNextSchedule(nextScheduleMins); bt.println();

  bt.print(F("Flow: ")); bt.println(String(flow) + " l/min");
  bt.print(F("Current pulses: ")); bt.println(pulseCount);
  bt.print(F("pumping: ")); bt.println(pumping);
  bt.print(F("measuring: ")); bt.println(measuring);
  
  bt.print(F("currentMeasurement.volume: ")); bt.println(String(currentMeasurementMl/1000) + " l");
  bt.print(F("currentMeasurement.start: ")); BT_printNextSchedule(currentMeasurementMin); bt.println();
  
  bt.print(F("ThresholdMeasureStart: ")); bt.println(ThresholdMeasureStart);
  bt.print(F("ThresholdMeasureStop: ")); bt.println(ThresholdMeasureStop);
  bt.print(F("ThresholdTurnOff: ")); bt.println(ThresholdTurnOff);
  bt.print(F("TresholdTurnOffSeconds: ")); bt.println(TresholdTurnOffSeconds);
  bt.print(F("RepeatIntervalMins: ")); bt.println(RepeatIntervalMins);
  
  bt.print(F("aboveThresholdMeasureTs: ")); bt.println(aboveThresholdMeasureTs/1000);
  bt.print(F("aboveThresholdTurnOffTs: ")); bt.println(aboveThresholdTurnOffTs/1000);
}

void SendInfoBlueTooth() {
  bt.print(F("Cas: ")); BT_printTime(dateTime); bt.println();
  bt.print(F("Rezim: ")); bt.println(Mode);
  bt.print(F("Prietok: ")); bt.println(String(flow) + " l/min");

  if (measuring){
    bt.print(F("Meranie: ")); 
    BT_printNextSchedule(currentMeasurementMin); 
    bt.print(F("  "));
    bt.print(GetCurrentMins() - currentMeasurementMin); bt.print("m ");
    bt.print(currentMeasurementMl/1000); bt.println("l");
  } else {
    bt.println(F("Meranie: neprebieha meranie"));
  }

  if (pumping) {
    bt.print(F("Prebieha cerpanie"));
  } else {
    if (Mode > MODE_MANUAL) {
      bt.print(F("Najblizsie cerpanie: ")); BT_printNextSchedule(nextScheduleMins); bt.println();
    } else {
      bt.println(F("Najblizsie cerpanie: nie je zapnuty planovaci rezim ")); 
    }        
  }

  // TBD last measurments (3x), daily measuremnt, yesterday
}
