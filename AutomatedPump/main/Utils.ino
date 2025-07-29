void Socket_On() {
  socket.send(Socket_B_On);
  aboveThresholdTurnOffTs = millis();
  bt.println(F("ZASUVKA ZAPNUTA"));
}

void Socket_Off() {
  socket.send(Socker_B_Off);
  bt.println(F("ZASUVKA VYPNUTA"));
}

void BT_printNextSchedule(uint32_t t) {
  unsigned int hours = (t%DAY_MINUTES)/60;
  unsigned int minutes = (t%DAY_MINUTES)%60;
  
  if (hours < 10) bt.print("0");
  bt.print(hours);
  bt.print(":");
  if (minutes < 10) bt.print("0");
  bt.print(minutes);
}

void BT_printDate(RTCDateTime dt) {
  if (dt.day < 10) bt.print("0");
  bt.print(dt.day);
  bt.print("/");
  
  if (dt.month < 10) bt.print("0");
  bt.print(dt.month);
  bt.print("/");

  bt.print(dt.year);
}

void BT_printTime(RTCDateTime dt) {
  if (dt.hour < 10) bt.print("0");
  bt.print(dt.hour);
  bt.print(":");
  
  if (dt.minute < 10) bt.print("0");
  bt.print(dt.minute);
  bt.print(":");

  if (dt.second < 10) bt.print("0");
  bt.print(dt.second);
}

void BT_printCurrentMeasurement(String caption, measurement *m) {
  m->to = GetCurrentMins();
  BT_printMeasurement(caption, m);
}

void BT_printMeasurement(String caption, measurement *m) {
  bt.print(caption);
  BT_printNextSchedule(m->from); 
  bt.print(F("  "));
  bt.print(m->to - m->from); bt.print("m ");
  bt.print(m->volume/1000); bt.println("l");
}
