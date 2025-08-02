#define RX 11
#define TX 10

#define SOCKET_PIN 9

#define FLOWMETER_PIN 2
#define FLOWMETER_INTERRUPT_PIN 0 // 0 = digitální pin 2

#define Socket_B_On "110011011110011010110100"
#define Socker_B_Off "110011100000001000100100"

//#define ENABLE_SYNTETIC_FLOW 1

#include <Wire.h>
#include <SoftwareSerial.h>
#include <DS3231.h>
#include <RCSwitch.h>
#include <EEPROM.h>

#define MODE_MEASURE 0
#define MODE_MANUAL 1
#define MODE_REPEAT 2

typedef struct {
  uint32_t from;
  uint32_t to;
  uint32_t volume; // in ml
} measurement;

struct Config {
  unsigned long tag;
  float pulseFormula_coefficient;
  float pulseFormula_offset;
  byte ThresholdMeasureStart;
  byte ThresholdMeasureStop;
  byte Mode;
  byte ThresholdTurnOff;
  byte TresholdTurnOffSeconds;
  long DelayStartPump;
  uint32_t RepeatIntervalMins;
};

Config config;

SoftwareSerial bt(TX, RX);
RCSwitch socket = RCSwitch();
DS3231 rtc;

RTCDateTime dateTime;

volatile byte pulseCount = 0;
float flow = 0.0;             // in liter/minute
unsigned int flowML = 0;      // in militers
unsigned long sumML = 0;      // in mililiters
unsigned long oldTime = 0;
unsigned long pulsesTotal = 0;

int SynteticPulses = 0;

// Calculate volume that flow in this loop.
// Formula for pulse meter: F = pulseFormula_coefficient * Q - pulseFormula_offset
// Other parameters: pulseFormula_coefficient=a, pulseFormula_offset=b, loopMillis=L, loopPulses=P, flow in loop = Vml
// 
// Formula derived by Gemini AI: Vml = (1000P+bL)/(60a)
// AI Prompt: 
//   You are math expert. Flow pulse meter is characterized by formula F = a* Q - b, where a and b are constants. F is frequency in Hertz, Q is flow in liters per minute.
//   We can measure number of pulses P in time interval L (provided in milliseconds). 
//   Provide formula for volume (in mililiters) in time interval L based on measures pulses and formula above.
unsigned int GetLoopFlowML(unsigned long loopMillis, unsigned long  loopPulses) {
  return (1000.0 * (float) loopPulses + config.pulseFormula_offset*loopMillis)/(60.0*config.pulseFormula_coefficient);
}

void setup() {
  Serial.begin(9600);
  
  bt.begin(9600);

  socket.enableTransmit(SOCKET_PIN);
  socket.setProtocol(4);
  socket.setRepeatTransmit(2);
  
  rtc.begin();

  // Set datetime as compiling time, this is static and should be applied only during code upload
  rtc.setDateTime(__DATE__, __TIME__);

  pinMode(FLOWMETER_PIN, INPUT);
  attachInterrupt(FLOWMETER_INTERRUPT_PIN, addPulse, FALLING);

  // we need time already here so we can schedule pumping correctly
  dateTime = rtc.getDateTime();
  
  loadConfiguration();
  InitController();
}

void loop() {
  byte bluetoothData;
  unsigned long loopTime = millis() - oldTime;

  if (loopTime > 500) {
    detachInterrupt(FLOWMETER_INTERRUPT_PIN);

    // reevaluate loop-time
    loopTime = millis() - oldTime;

    dateTime = rtc.getDateTime();

#ifdef ENABLE_SYNTETIC_FLOW
    pulseCount = SynteticPulses;
#endif

    flowML = GetLoopFlowML(loopTime, pulseCount);
    flow = ( (float) flowML * (60000.0 / (float) loopTime))/1000.0;    // from flowML in this loop calculate flow in liters/minute
    sumML += flowML;
    pulsesTotal += pulseCount;

    ProcessFlowData();
    
    pulseCount = 0;
    oldTime = millis();
    attachInterrupt(FLOWMETER_INTERRUPT_PIN, addPulse, FALLING);
  }
  
  if (bt.available() > 0) {
    String command = bt.readStringUntil('\n');
    command.trim();

    if (command.startsWith("Set")) {
      int firstSpace = command.indexOf(' ');
      int secondSpace = command.indexOf(' ', firstSpace + 1);
      if (firstSpace > 0 && secondSpace > 0) {
        int slot = command.substring(firstSpace + 1, secondSpace).toInt();
        float value = command.substring(secondSpace + 1).toFloat();
        
        switch (slot) {
          case 1:
            config.pulseFormula_coefficient = value;
            bt.print(F("New pulseFormula_coefficient: "));
            bt.println(config.pulseFormula_coefficient);
            break;
          case 2:
            config.pulseFormula_offset = value;
            bt.print(F("New pulseFormula_offset: "));
            bt.println(config.pulseFormula_offset);
            break;
          case 3:
            config.ThresholdMeasureStart = (byte)value;
            bt.print(F("New ThresholdMeasureStart: "));
            bt.println(config.ThresholdMeasureStart);
            break;
          case 4:
            config.ThresholdMeasureStop = (byte)value;
            bt.print(F("New ThresholdMeasureStop: "));
            bt.println(config.ThresholdMeasureStop);
            break;
          case 5:
            config.Mode = (byte)value;
            bt.print(F("New Mode: "));
            bt.println(config.Mode);
            break;
          case 6:
            config.ThresholdTurnOff = (byte)value;
            bt.print(F("New ThresholdTurnOff: "));
            bt.println(config.ThresholdTurnOff);
            break;
          case 7:
            config.TresholdTurnOffSeconds = (byte)value;
            bt.print(F("New TresholdTurnOffSeconds: "));
            bt.println(config.TresholdTurnOffSeconds);
            break;
          case 8:
            config.DelayStartPump = (long)value;
            bt.print(F("New DelayStartPump: "));
            bt.println(config.DelayStartPump);
            break;
          case 9:
            config.RepeatIntervalMins = (uint32_t)value;
            bt.print(F("New RepeatIntervalMins: "));
            bt.println(config.RepeatIntervalMins);
            break;
          case 100:
            {
              int timeVal = (int)value;
              int hours = timeVal / 100;
              int minutes = timeVal % 100;
              UpdateSchedule(hours, minutes);
            }
            break;
          default:
            bt.print(F("Unknown slot: "));
            bt.println(slot);
        }
      } else {
        bt.println(F("Invalid Set command format."));
      }
    } else {
      switch (command.charAt(0)) {
        case '0':
          Manual_Off();  
          break;
        case '1':
          Manual_On();
          break;
        case 'm':
          SwitchMode();
          break;
        case 'a':
          SendStatusBlueTooth();
          break;
        case 's':
          SendInfoBlueTooth(false);
          break;
        case 'S':
          SendInfoBlueTooth(true);
          break;
        case 'F':
          UpdateSynteticPulses(5);
          break;
        case 'f':
          UpdateSynteticPulses(-5);
          break;
        case 'R':
          UpdateRepeatInterval(10);
          break;
        case 'r':
          UpdateRepeatInterval(-10);
          break;
        case 'X':
          saveConfiguration();
          bt.println("Configuration saved.");
          break;
        default:
          bt.print(F("Unknown command: "));
          bt.println(command);
      }
    }
  }
  
  delay(100);
}

void UpdateSynteticPulses(int delta) {
  int p = SynteticPulses + delta;
  if (p >= 0 && p < 400)
    SynteticPulses = p;
  bt.print(F("Novy syntetic pulse: "));
  bt.println(SynteticPulses);
}

void addPulse() {
  pulseCount++;
}
