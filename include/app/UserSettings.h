#pragma once

struct UserSettings {
  bool displayFlipped180 = false;
};

namespace UserSettingsStore {

bool load(UserSettings& settings);
bool save(const UserSettings& settings);
bool saveDisplayFlipped180(bool flipped);
bool isDisplayFlipped180();

}  // namespace UserSettingsStore
