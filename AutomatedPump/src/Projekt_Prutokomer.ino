#define RX 11
#define TX 10

#define SOCKET_PIN 9

#define FLOWMETER_PIN 2
#define FLOWMETER_INTERRUPT_PIN 0 // 0 = digitální pin 2

#define Socket_B_On "110011011110011010110100"
#define Socker_B_Off "110011100000001000100100"

#include <Wire.h>
#include <SoftwareSerial.h>
#include <DS3231.h>
#include <RCSwitch.h>
#include <LiquidCrystal_I2C.h>

SoftwareSerial bluetooth(TX, RX);
RCSwitch socket = RCSwitch();
DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 20, 4);

RTCDateTime dateTime;

const float calibrationFactor = 3.96;

volatile byte pulseCount = 0;
float flow = 0.0;
unsigned int flowML = 0;
unsigned long sumML = 0;
unsigned long oldTime = 0;
unsigned long pulsesTotal = 0;

void setup() {
  Serial.begin(9600);
  
  bluetooth.begin(9600);

  socket.enableTransmit(SOCKET_PIN);
  socket.setProtocol(4);
  socket.setRepeatTransmit(2);
  
  rtc.begin();

  pinMode(FLOWMETER_PIN, INPUT);
  attachInterrupt(FLOWMETER_INTERRUPT_PIN, addPulse, FALLING);

  lcd.begin();
  lcd.backlight();

  lcd.print(F("PRUTOKOMER v0.2"));
  
  delay(2000);
  lcd.clear();

  InitController();
}

void loop() {
  byte bluetoothData;

  if ((millis() - oldTime) > 1000) {
    dateTime = rtc.getDateTime();
    
    detachInterrupt(FLOWMETER_INTERRUPT_PIN);

    // mine: 198 Hz (Q – 30 l/min), 99 Hz (Q – 15 l/min), 33 Hz (Q – 5 l/min); F = 6,6 * Q;   396 pulzu / litr
    flow = (pulseCount * (float) 60000) / (396 * (millis() - oldTime));
    flowML = (flow*1000) / 60;
    
    sumML += flowML;
    pulsesTotal += pulseCount;

    LCD_updateFlow(flow);

    ProcessFlowData();
    
    pulseCount = 0;
    oldTime = millis();
    attachInterrupt(FLOWMETER_INTERRUPT_PIN, addPulse, FALLING);
  }
  
  if (bluetooth.available() > 0) {
    // načtení prvního znaku ve frontě do proměnné
    bluetoothData=bluetooth.read();
  
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
      case 's':
        SendStatusBlueTooth();
        break;
      case 'b':
        // zde je ukázka načtení většího počtu informací,
        // po přijetí znaku 'b' tedy postupně tiskneme 
        // další znaky poslané ve zprávě
        bluetooth.print(F("Nacitam zpravu: "));
        bluetoothData=bluetooth.read();
        // v této smyčce zůstáváme do té doby,
        // dokud jsou nějaké znaky ve frontě
        while (bluetooth.available() > 0) {
          bluetooth.write(bluetoothData);
          // krátká pauza mezi načítáním znaků
          delay(10);
          bluetoothData=bluetooth.read();
        }
        bluetooth.println();
        break;
      case '\r':
        // přesun na začátek řádku - znak CR
        break;
      case '\n':
        // odřádkování - znak LF
        break;
      default:
        BT_print(F("Unknown commamd: ")); BT_println(String(bluetoothData));
    }
  }
  
  delay(100);
}

void addPulse() {
  pulseCount++;
}
