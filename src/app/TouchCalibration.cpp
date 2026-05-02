#include "app/TouchCalibration.h"

#include <Preferences.h>

namespace {

constexpr const char* Namespace = "touch_cal";
constexpr uint32_t Magic = 0x43594454UL;  // CYDT
constexpr uint16_t Version = 1;

constexpr const char* MagicKey = "magic";
constexpr const char* VersionKey = "version";
constexpr const char* RawMinXKey = "raw_min_x";
constexpr const char* RawMaxXKey = "raw_max_x";
constexpr const char* RawMinYKey = "raw_min_y";
constexpr const char* RawMaxYKey = "raw_max_y";
constexpr const char* OffsetXKey = "offset_x";
constexpr const char* OffsetYKey = "offset_y";
constexpr const char* SwapXYKey = "swap_xy";
constexpr const char* InvertXKey = "invert_x";
constexpr const char* InvertYKey = "invert_y";

bool isValid(const TouchCalibrationData& calibration) {
  return calibration.rawMaxX > calibration.rawMinX &&
         calibration.rawMaxY > calibration.rawMinY &&
         calibration.rawMaxX - calibration.rawMinX > 1000 &&
         calibration.rawMaxY - calibration.rawMinY > 1000;
}

}  // namespace

namespace TouchCalibrationStore {

bool load(TouchCalibrationData& calibration) {
  Preferences preferences;
  if (!preferences.begin(Namespace, false)) {
    return false;
  }

  const bool validHeader = preferences.getUInt(MagicKey, 0) == Magic &&
                           preferences.getUShort(VersionKey, 0) == Version;
  if (!validHeader) {
    preferences.end();
    return false;
  }

  calibration.rawMinX = preferences.getInt(RawMinXKey, 0);
  calibration.rawMaxX = preferences.getInt(RawMaxXKey, 0);
  calibration.rawMinY = preferences.getInt(RawMinYKey, 0);
  calibration.rawMaxY = preferences.getInt(RawMaxYKey, 0);
  calibration.offsetX = preferences.getShort(OffsetXKey, 0);
  calibration.offsetY = preferences.getShort(OffsetYKey, 0);
  calibration.swapXY = preferences.getBool(SwapXYKey, false);
  calibration.invertX = preferences.getBool(InvertXKey, false);
  calibration.invertY = preferences.getBool(InvertYKey, false);

  preferences.end();
  return isValid(calibration);
}

bool save(const TouchCalibrationData& calibration) {
  if (!isValid(calibration)) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(Namespace, false)) {
    return false;
  }

  preferences.putUInt(MagicKey, Magic);
  preferences.putUShort(VersionKey, Version);
  preferences.putInt(RawMinXKey, calibration.rawMinX);
  preferences.putInt(RawMaxXKey, calibration.rawMaxX);
  preferences.putInt(RawMinYKey, calibration.rawMinY);
  preferences.putInt(RawMaxYKey, calibration.rawMaxY);
  preferences.putShort(OffsetXKey, calibration.offsetX);
  preferences.putShort(OffsetYKey, calibration.offsetY);
  preferences.putBool(SwapXYKey, calibration.swapXY);
  preferences.putBool(InvertXKey, calibration.invertX);
  preferences.putBool(InvertYKey, calibration.invertY);

  preferences.end();
  return true;
}

bool clear() {
  Preferences preferences;
  if (!preferences.begin(Namespace, false)) {
    return false;
  }

  const bool ok = preferences.clear();
  preferences.end();
  return ok;
}

}  // namespace TouchCalibrationStore
