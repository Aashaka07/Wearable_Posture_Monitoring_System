// imu_pair.h - two MPU6050 (upper back 0x68, lower back 0x69). Replaces flex sensor.
// Same file lives in posture_logger/, posture_ml/ and posture_monitor/.
#pragma once
#include <Wire.h>

#define SDA_PIN    21
#define SCL_PIN    22
#define ADDR_UPPER 0x68   // AD0 -> GND
#define ADDR_LOWER 0x69   // AD0 -> 3.3V
#define ALPHA      0.98f  // complementary filter weight

struct Imu { uint8_t addr; float roll, pitch, baseRoll, basePitch; bool primed; };
struct Features { float rollDev, pitchDev, lowRollDev, lowPitchDev, spinePitchDev, spineRollDev; };

static Imu imuU = {ADDR_UPPER, 0, 0, 0, 0, false};
static Imu imuL = {ADDR_LOWER, 0, 0, 0, 0, false};
static float baseSpP = 0, baseSpR = 0;
static uint32_t lastUs = 0;

static bool wr(uint8_t a, uint8_t r, uint8_t v) {
  Wire.beginTransmission(a); Wire.write(r); Wire.write(v);
  return Wire.endTransmission() == 0;
}

static bool imuInit(Imu &m) {
  return wr(m.addr, 0x6B, 0x00) && wr(m.addr, 0x1C, 0x00) &&
         wr(m.addr, 0x1B, 0x00) && wr(m.addr, 0x1A, 0x03);
}

static bool imuRead(Imu &m, float dt) {
  Wire.beginTransmission(m.addr); Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)m.addr, 14) != 14) return false;
  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();
  int16_t gx = (Wire.read() << 8) | Wire.read();
  int16_t gy = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();
  float fx = ax / 16384.0f, fy = ay / 16384.0f, fz = az / 16384.0f;
  float accRoll  = atan2f(fy, fz) * 180.0f / PI;
  float accPitch = atan2f(-fx, sqrtf(fy * fy + fz * fz)) * 180.0f / PI;
  if (!m.primed) { m.roll = accRoll; m.pitch = accPitch; m.primed = true; return true; }
  m.roll  = ALPHA * (m.roll  + (gx / 131.0f) * dt) + (1 - ALPHA) * accRoll;
  m.pitch = ALPHA * (m.pitch + (gy / 131.0f) * dt) + (1 - ALPHA) * accPitch;
  return true;
}

static bool imuPairBegin() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  delay(200);
  bool u = imuInit(imuU), l = imuInit(imuL);
  Serial.printf("# upper(0x68): %s, lower(0x69): %s\n", u ? "OK" : "FAIL", l ? "OK" : "FAIL");
  lastUs = micros();
  return u && l;
}

static bool imuPairUpdate() {
  uint32_t now = micros();
  float dt = (now - lastUs) / 1e6f; lastUs = now;
  if (dt <= 0 || dt > 0.2f) dt = 0.02f;
  bool a = imuRead(imuU, dt), b = imuRead(imuL, dt);
  return a && b;
}

// Sit upright, then call: averages ~1 s as the "good posture" baseline.
static void imuPairCalibrate() {
  for (int i = 0; i < 25; i++) { delay(20); imuPairUpdate(); }
  const int N = 50;
  float ur = 0, up = 0, lr = 0, lp = 0;
  for (int i = 0; i < N; i++) {
    delay(20); imuPairUpdate();
    ur += imuU.roll; up += imuU.pitch; lr += imuL.roll; lp += imuL.pitch;
  }
  imuU.baseRoll = ur / N; imuU.basePitch = up / N;
  imuL.baseRoll = lr / N; imuL.basePitch = lp / N;
  baseSpP = imuU.basePitch - imuL.basePitch;
  baseSpR = imuU.baseRoll  - imuL.baseRoll;
}

static float wrap180(float a) { while (a > 180) a -= 360; while (a < -180) a += 360; return a; }

static Features imuPairFeatures() {
  Features f;
  f.rollDev       = wrap180(imuU.roll  - imuU.baseRoll);
  f.pitchDev      = wrap180(imuU.pitch - imuU.basePitch);
  f.lowRollDev    = wrap180(imuL.roll  - imuL.baseRoll);
  f.lowPitchDev   = wrap180(imuL.pitch - imuL.basePitch);
  f.spinePitchDev = wrap180((imuU.pitch - imuL.pitch) - baseSpP);   // replaces flex
  f.spineRollDev  = wrap180((imuU.roll  - imuL.roll)  - baseSpR);
  return f;
}
