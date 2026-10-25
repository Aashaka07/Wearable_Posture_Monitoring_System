#include "imu_pair.h"

const uint32_t SAMPLE_MS = 20;   // 50 Hz
int label = 0;                   // 0 good, 1 slouch, 2 side lean

void setup() {
  Serial.begin(115200);
  while (!imuPairBegin()) { Serial.println("# IMU init failed - check wiring / AD0"); delay(1000); }
  imuPairCalibrate();
  Serial.println("# calibrated");
  Serial.println("t_ms,label,rollDev,pitchDev,lowRollDev,lowPitchDev,spinePitchDev,spineRollDev");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c >= '0' && c <= '2') label = c - '0';
    else if (c == 'c') { imuPairCalibrate(); Serial.println("# calibrated"); }
  }
  static uint32_t last = 0;
  if (millis() - last < SAMPLE_MS) return;
  last = millis();
  if (!imuPairUpdate()) { Serial.println("# IMU read failed"); return; }
  Features f = imuPairFeatures();
  Serial.printf("%lu,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", millis(), label,
                f.rollDev, f.pitchDev, f.lowRollDev, f.lowPitchDev, f.spinePitchDev, f.spineRollDev);
}
