
#define MODE_MEASURE 0
#define MODE_MANUAL 1
#define MODE_REPEAT 2

#define MODES_COUNT 3

#define MEASUREMENT_LIST_SIZE 5

#define DAY_MINUTES 1440
#define TIMEZONE_COMPENSATION 60  // library has bug/feature, unixtime is in UTC so we need to add 1 hour

byte Mode = 0;

byte ThresholdMeasureStart = 5; // measure when flow is above this value (in l/min)
byte ThresholdMeasureStop = 2; // stop measure when flow is below in this valie (in l/min)

byte ThresholdTurnOff = 10; // turn off when flow below this value (in l/min) 
byte TresholdTurnOffSeconds = 3; // turn off when flow is below ThresholdTurnOff for more this value (in seconds)
long DelayStartPump = 0;  // (in seconds) when pump start this delay interval will postpone evaluation of TresholdTurnOffSeconds. Idea behind: it takes time for pump to trigger flow, longer than 5 sec..

uint32_t RepeatIntervalMins = 120;

long aboveThresholdMeasureTs = 0;
long aboveThresholdTurnOffTs = 0;

uint32_t nextScheduleMins = 0;
bool measuring = false;
bool pumping = false;

measurement current, history[MEASUREMENT_LIST_SIZE];
measurement today, yesterday;

unsigned long totalVolume;
unsigned long totalMeasurements;

byte historySize = 0;

void ProcessFlowData() {
  long ts = millis();
  if (flow > ThresholdMeasureStop)  aboveThresholdMeasureTs = ts;
  if (flow > ThresholdTurnOff)  aboveThresholdTurnOffTs = ts;

  if (pumping && Mode > MODE_MEASURE && (ts - aboveThresholdTurnOffTs) > (long) TresholdTurnOffSeconds * 1000) { 
    bt.println(F("Nizky prietok pri pustenom cerpadle, zastavujem cerpadlo!"));
    StopPump();
  }
  
  CheckSchedule();
  
  if (measuring && ThresholdMeasureStop > flow) { 
    // finish measurement
    bt.println(F("Meranie zastavene, nizky prietok."));

    current.to = GetCurrentMins();
    
    ProcessMeasurement(&current);
    
    measuring = false;
 
  } else if (!measuring && ThresholdMeasureStart < flow)  {
    // start measure
    bt.println(F("Zacinam meranie, zisteni vyssi prietok."));
    
    measuring = true;
    current.volume = 0;
    current.from = GetCurrentMins();
  }

  if (measuring) {
    current.volume += flowML;
  }
}

void InitController() {
  SetMode(MODE_REPEAT);

  // load data from EEPROM
  EEPROM_LoadData();
  
  totalVolume = storage.totalVolume;
  totalMeasurements = storage.totalMeasurements;
    
  // init daily measurements
  uint32_t noon = (GetCurrentMins()/DAY_MINUTES)*DAY_MINUTES;
  
  today.from = noon;
  today.to = noon + DAY_MINUTES - 1;
  today.volume = 0;
  
  yesterday.from = noon - DAY_MINUTES;
  yesterday.to = noon - 1;
  yesterday.volume = 0;
}

void SetMode(byte mode) {
  Mode = mode % MODES_COUNT;
  
  bt.print(F("Zmena rezimu, novy rezim: ")); bt.println(Mode);

  // init mode
  switch(Mode) {
    case MODE_REPEAT:
      // we need to schedule new pump from current, not from existing schedule (that can be in the future)
      nextScheduleMins = GetCurrentMins();
      break;
    default:
      break;
  }
  ScheduleNextIteration();
}

void SwitchMode() {
  SetMode(Mode+1);
}

void ScheduleNextIteration() {
  if (Mode == MODE_MEASURE || Mode == MODE_MANUAL) return;
  if (Mode == MODE_REPEAT) {
      // we are scheduling next pump based on last schedule BUT if last schedule is far in past we will schedule from current time
      while ((nextScheduleMins + RepeatIntervalMins) < GetCurrentMins()) {
        nextScheduleMins += RepeatIntervalMins;
      }
        
      nextScheduleMins = nextScheduleMins + RepeatIntervalMins;
  }
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

void CheckSchedule() {
  // do not check schedule while pumping
  if (pumping) return;
  
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

  nextScheduleMins = GetCurrentMins();
}

void UpdateRepeatInterval(int delta) {
  uint32_t p = RepeatIntervalMins + delta;
  if (p > 0 && p < 600)
    RepeatIntervalMins = p;
  bt.print(F("Novy interval spustania cerpadla (min): "));
  bt.println(RepeatIntervalMins);
}

void ProcessMeasurement(measurement *m) {
  totalVolume += m->volume;
  totalMeasurements++;

  storage.totalVolume = totalVolume;
  storage.totalMeasurements = totalMeasurements;
  EEPROM_SaveData();
  
  AddMeasurementToHistory(&current);

  // day change if measurement is in new day
  if (today.to < GetCurrentMins()) {
    uint32_t noon = GetNoonMins();
    
    yesterday = today;

    today.from = noon;
    today.to = noon + DAY_MINUTES -1;
    today.volume = 0;
  }
  
  today.volume += m->volume;
}

void AddMeasurementToHistory(measurement *m) {
  for(int i=MEASUREMENT_LIST_SIZE-2;i>=0;i--) {
    history[i+1] = history[i];
  }
  history[0] = *m;
  
  historySize++;
  if (historySize >= MEASUREMENT_LIST_SIZE) historySize=MEASUREMENT_LIST_SIZE;
}

uint32_t GetCurrentMins() {
  return (dateTime.unixtime / 60) + TIMEZONE_COMPENSATION;
}

uint32_t GetNoonMins() {
  return (GetCurrentMins()/DAY_MINUTES)*DAY_MINUTES;
}


void SendStatusBlueTooth() {
  // TBD: lot of stuff to send on blueetooth
  bt.print(F("uptime: ")); bt.print(millis()/1000); bt.println(F(" secs."));
  bt.print(F("date: ")); BT_printDate(dateTime); bt.println();
  bt.print(F("time: ")); BT_printTime(dateTime); bt.println();
  bt.print(F("getCurrentMins: ")); bt.println(GetCurrentMins());
  bt.print(F("getCurrentMins (formatted): ")); BT_printNextSchedule(GetCurrentMins()); bt.println();
    
  bt.print(F("mode: ")); bt.println(Mode);
  bt.print(F("next schedule: ")); BT_printNextSchedule(nextScheduleMins); bt.println();

  bt.print(F("flow: ")); bt.println(String(flow) + " l/min");
  bt.print(F("current pulses: ")); bt.println(pulseCount);
  bt.print(F("flowML: ")); bt.println(flowML);
  bt.print(F("totalVolume (l): ")); bt.println(totalVolume/1000);
  
  bt.print(F("pumping: ")); bt.println(pumping);
  bt.print(F("measuring: ")); bt.println(measuring);

  bt.print(F("currentMeasurement.from: ")); BT_printNextSchedule(current.from); bt.println();
  bt.print(F("currentMeasurement.volume: ")); bt.print(current.volume); bt.println(F(" ml"));
  
  bt.print(F("ThresholdMeasureStart: ")); bt.println(ThresholdMeasureStart);
  bt.print(F("ThresholdMeasureStop: ")); bt.println(ThresholdMeasureStop);
  bt.print(F("ThresholdTurnOff: ")); bt.println(ThresholdTurnOff);
  bt.print(F("TresholdTurnOffSeconds: ")); bt.println(TresholdTurnOffSeconds);
  bt.print(F("RepeatIntervalMins: ")); bt.println(RepeatIntervalMins);
  bt.print(F("DelayStartPump: ")); bt.println(DelayStartPump);
  bt.print(F("SynteticPulses: ")); bt.println(SynteticPulses);

  bt.print(F("storage.totalVolume: ")); bt.println(storage.totalVolume);
  bt.print(F("storage.totalMeasurements: ")); bt.println(storage.totalMeasurements);
  
  bt.print(F("aboveThresholdMeasureTs: ")); bt.println(aboveThresholdMeasureTs/1000);
  bt.print(F("aboveThresholdTurnOffTs: ")); bt.println(aboveThresholdTurnOffTs/1000);
  
  bt.print(F("totalVolume: ")); bt.println(totalVolume);
  bt.print(F("totalMeasurements: ")); bt.println(totalMeasurements);
  
  bt.print(F("free memory: ")); bt.println(freeMemory());
}

void SendInfoBlueTooth(bool full) {
  bt.print(F("Cas: ")); BT_printTime(dateTime); bt.println();
  
  bt.print(F("Rezim: "));
  switch(Mode) {
    case MODE_MEASURE:
      bt.println(F("Meranie"));
      break;
    case MODE_MANUAL:
      bt.println(F("Manual"));
      break; 
    case MODE_REPEAT:
      bt.print(F("Auto (")); bt.print(RepeatIntervalMins); bt.println(F(" m)"));
      break;     
  }
  
  bt.print(F("Prietok: ")); bt.println(String(flow) + " l/min");

  if (measuring){
    BT_printCurrentMeasurement(F("Meranie: "), &current);
  } else {
    bt.println(F("Meranie: neprebieha meranie"));
  }
  
  if (pumping) {
    bt.println(F("Prebieha cerpanie"));
  } else {
    if (Mode > MODE_MANUAL) {
      bt.print(F("Najblizsie cerpanie: ")); 
      BT_printNextSchedule(nextScheduleMins); 
      bt.println();
    } else {
      bt.println(F("Najblizsie cerpanie: nie je zapnuty planovaci rezim ")); 
    }        
  }

  if (full) {
    bt.print(F("Historia merani:  ")); bt.print(historySize); bt.println(F(" merani"));
    for(int i=0;i<historySize;i++) {
      BT_printMeasurement(F("  "), &history[i]);
    }

    uint32_t todayNoon = GetNoonMins();
    uint32_t yestedayNoon = todayNoon - DAY_MINUTES;

    // TBD: REFACTORING
    // this is very dirty but I need to handle case when there is no measurement today yet and today variable actually represent yesteday
    // needs fix - ensuring today is alwasy today, handle day switch diffetently
    if (today.from == todayNoon) {
      bt.print(F("DNES (l): ")); bt.println(today.volume/1000); 
    }
    if (today.from == yestedayNoon) {
      bt.print(F("DNES (l): ")); bt.println(yesterday.volume/1000); 
    }
    if (yesterday.from == yestedayNoon) {
      bt.print(F("VCERA (l): ")); bt.println(yesterday.volume/1000); 
    }
  }
}
