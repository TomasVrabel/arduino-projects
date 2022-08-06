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

typedef struct {
  uint32_t from;
  uint32_t to;
  unsigned int volume; // in ml
} measurement;

SoftwareSerial bt(TX, RX);
RCSwitch socket = RCSwitch();
DS3231 rtc;

RTCDateTime dateTime;

const float calibrationFactor = 3.96;

volatile byte pulseCount = 0;
float flow = 0.0;
unsigned int flowML = 0;
unsigned long sumML = 0;
unsigned long oldTime = 0;
unsigned long pulsesTotal = 0;

int SynteticPulses = 0;

void setup() {
  Serial.begin(9600);
  
  bt.begin(9600);

  socket.enableTransmit(SOCKET_PIN);
  socket.setProtocol(4);
  socket.setRepeatTransmit(2);
  
  rtc.begin();

  pinMode(FLOWMETER_PIN, INPUT);
  attachInterrupt(FLOWMETER_INTERRUPT_PIN, addPulse, FALLING);

  // we need time already here so we can schedule pumping correctly
  dateTime = rtc.getDateTime();
  
  InitController();
}

void loop() {
  byte bluetoothData;

  if ((millis() - oldTime) > 1000) {
    dateTime = rtc.getDateTime();
    
    detachInterrupt(FLOWMETER_INTERRUPT_PIN);

#ifdef ENABLE_SYNTETIC_FLOW
    pulseCount = SynteticPulses;
#endif

    // mine: 198 Hz (Q – 30 l/min), 99 Hz (Q – 15 l/min), 33 Hz (Q – 5 l/min); F = 6,6 * Q;   396 pulzu / litr
    flow = (pulseCount * (float) 60000) / (396 * (millis() - oldTime));
    flowML = (((uint32_t) pulseCount) * 1000) / 396;
    
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
