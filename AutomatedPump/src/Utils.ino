void Socket_On() {
  socket.send(Socket_B_On);
  aboveThresholdTurnOffTs = millis();
  BT_println(F("SOCKET ON"));
}

void Socket_Off() {
  socket.send(Socker_B_Off);
  BT_println(F("SOCKET OFF"));
}

String FormatDate(RTCDateTime *dateTime, bool includeYear) {
  if (includeYear) {
    return String(dateTime->day) + "/" + String(dateTime->month) + "/" + String(dateTime->year);
  } else {
    return String(dateTime->day) + "/" + String(dateTime->month);
  }
}

String FormatTime(RTCDateTime *dateTime, bool includeSeconds) {
  if (includeSeconds) {
    return String(dateTime->hour) + ":" + String(dateTime->minute) + ":" + String(dateTime->second);
  } else {
    return String(dateTime->hour) + ":" + String(dateTime->minute);
  }
}

// 16 letters ??
void LCD_updateFlow(float flow) {
   lcd.setCursor ( 0, 0 );
   lcd.print("T "); lcd.print(flow); lcd.print(" l/m");
}

String STR_fixLength(String str, byte len) {
  while (str.length() < len)  {
    str = str + " ";
  }
  return str.substring(0, len);
}

void LCD_updateCurrentMeasurement(bool measuring, int minutes, byte duration, int volume) {
   lcd.setCursor ( 0, 1 );
   if (!measuring) {
      lcd.print(STR_fixLength("Ziadny prietok.", 20));
   } else {
     lcd.print(STR_fixLength("M " + String(minutes/60) + ":" + String(minutes%60) + " " + String(duration) + "m " + volume + "l   ", 20));
   }
}

void LCD_updateLastMeasurement(int minutes, byte duration, int volume) {
   lcd.setCursor ( 0, 2 );
   lcd.print(STR_fixLength("P " + String(minutes/60) + ":" + String(minutes%60) + " " + String(duration) + "m " + volume + "l   ", 20));
}

void LCD_updateMode() {
   lcd.setCursor ( 12, 0 );
   switch(Mode) {
    case MODE_MEASURE:
      lcd.print(STR_fixLength("Meranie", 8));
      break;
    case MODE_MANUAL:
      lcd.print(STR_fixLength("Manual",8));
      break;
    case MODE_REPEAT:
      lcd.print("A:"); lcd.print(RepeatIntervalMins);
      break;
    case MODE_SCHEDULE:
      lcd.print("A:Plan");
      break;
   }
}

void BT_print(String message) {
  bluetooth.print(message);
}

void BT_println(String message) {
  bluetooth.println(message);
}

void BT_println() {
  bluetooth.println("");
}
