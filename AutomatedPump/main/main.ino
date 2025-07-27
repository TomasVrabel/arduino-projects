#define RX 11
#define TX 10

#define SOCKET_PIN 9

#define FLOWMETER_PIN 2
#define FLOWMETER_INTERRUPT_PIN 0 // 0 = digitální pin 2

#define Socket_B_On "110011011110011010110100"
#define Socker_B_Off "110011100000001000100100"

#define STORAGE_ADDRESS 0x00

//#define ENABLE_SYNTETIC_FLOW 1

#include <Wire.h>
#include <SoftwareSerial.h>
#include <DS3231.h>
#include <RCSwitch.h>
#include <EEPROM.h>

typedef struct {
  uint32_t from;
  uint32_t to;
  uint32_t volume; // in ml
} measurement;

struct storage_model {
  unsigned long totalVolume; 
  unsigned long totalMeasurements;
} storage;

SoftwareSerial bt(TX, RX);
RCSwitch socket = RCSwitch();
DS3231 rtc;

RTCDateTime dateTime;

const float pulseFormula_coefficient = 8.0;
const float pulseFormula_offset = 4.0;

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
  return (1000.0 * (float) loopPulses + pulseFormula_offset*loopMillis)/(60.0*pulseFormula_coefficient);
}

void setup() {
  Serial.begin(9600);
  
  bt.begin(9600);

  socket.enableTransmit(SOCKET_PIN);
  socket.setProtocol(4);
  socket.setRepeatTransmit(2);
  
  rtc.begin();

  // Set datetime as compiling time
  rtc.setDateTime(__DATE__, __TIME__);

  pinMode(FLOWMETER_PIN, INPUT);
  attachInterrupt(FLOWMETER_INTERRUPT_PIN, addPulse, FALLING);

  // we need time already here so we can schedule pumping correctly
  dateTime = rtc.getDateTime();
  
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
    // načtení prvního znaku ve frontě do proměnné
    bluetoothData=bt.read();
    
    // dekódování přijatého znaku
    switch (bluetoothData) {
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
        for(int i=0;i<sizeof(storage_model);i++) EEPROM.update(STORAGE_ADDRESS + i, 0);
        bt.println("Storage reset");
        break;
      case 'b':
        // zde je ukázka načtení většího počtu informací,
        // po přijetí znaku 'b' tedy postupně tiskneme 
        // další znaky poslané ve zprávě
        bt.print(F("Nacitam zpravu: "));
        bluetoothData=bt.read();
        // v této smyčce zůstáváme do té doby,
        // dokud jsou nějaké znaky ve frontě
        while (bt.available() > 0) {
          bt.write(bluetoothData);
          // krátká pauza mezi načítáním znaků
          delay(10);
          bluetoothData=bt.read();
        }
        bt.println();
        break;
      case '\r':
        // přesun na začátek řádku - znak CR
        break;
      case '\n':
        // odřádkování - znak LF
        break;
      default:
        bt.print(F("Unknown commamd: ")); bt.println(bluetoothData);
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
