#pragma once
#include <Arduino.h>

// คำนวณค่าไฟ: Current × Voltage × Rate (บาท)
class EnergyTracker {
public:
  explicit EnergyTracker(float rate)
    : _rate(rate), _startKwh(0.0f), _dayStart(0) {}

  void begin(float kwh) { _startKwh = kwh; _dayStart = millis(); }

  void update(float totalKwh) {
    if (millis() - _dayStart >= 86400000UL) {
      _startKwh = totalKwh;
      _dayStart  = millis();
    }
  }

  float dailyKwh(float t)    const { return t - _startKwh; }
  float dailyCost(float t)   const { return dailyKwh(t) * _rate; }
  float monthlyCost(float t) const { return dailyCost(t) * 30.0f; }

  static const char* classify(float w) {
    if (w >= 3000) return "HIGH";
    if (w >= 1000) return "MEDIUM";
    return "LOW";
  }
private:
  float         _rate, _startKwh;
  unsigned long _dayStart;
};
