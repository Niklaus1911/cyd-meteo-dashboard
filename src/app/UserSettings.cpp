#include "app/UserSettings.h"

#include <Preferences.h>

namespace {

constexpr const char* Namespace = "ui_settings";
constexpr const char* DisplayFlipKey = "disp_flip180";

}  // namespace

namespace UserSettingsStore {

bool load(UserSettings& settings) {
  Preferences preferences;
  if (!preferences.begin(Namespace, false)) {
    return false;
  }

  settings.displayFlipped180 = preferences.getBool(DisplayFlipKey, false);
  preferences.end();
  return true;
}

bool save(const UserSettings& settings) {
  Preferences preferences;
  if (!preferences.begin(Namespace, false)) {
    return false;
  }

  preferences.putBool(DisplayFlipKey, settings.displayFlipped180);
  preferences.end();
  return true;
}

bool saveDisplayFlipped180(bool flipped) {
  UserSettings settings;
  settings.displayFlipped180 = flipped;
  return save(settings);
}

bool isDisplayFlipped180() {
  UserSettings settings;
  load(settings);
  return settings.displayFlipped180;
}

}  // namespace UserSettingsStore
