#include "app/MqttSettings.h"

#include <Preferences.h>
#include <cstdlib>
#include <cstring>

namespace {

constexpr const char* Namespace = "mqtt";
constexpr const char* HostKey = "host";
constexpr const char* PortKey = "port";
constexpr const char* UserKey = "user";
constexpr const char* PassKey = "pass";
constexpr const char* TopicKey = "topic";

void copyString(char* destination, size_t destinationSize, const String& source) {
  if (destinationSize == 0) {
    return;
  }

  strncpy(destination, source.c_str(), destinationSize - 1);
  destination[destinationSize - 1] = '\0';
}

bool isNoneValue(const char* value) {
  return value != nullptr && strcasecmp(value, "none") == 0;
}

}  // namespace

bool MqttSettings::isComplete() const {
  return host[0] != '\0' && telemetryTopic[0] != '\0' && port > 0;
}

bool MqttSettings::hasCredentials() const {
  return username[0] != '\0' && !isNoneValue(username);
}

namespace MqttSettingsStore {

bool load(MqttSettings& settings) {
  Preferences preferences;
  if (!preferences.begin(Namespace, true)) {
    return false;
  }

  copyString(settings.host, sizeof(settings.host), preferences.getString(HostKey, ""));
  settings.port = preferences.getUShort(PortKey, AppConfig::DefaultMqttPort);
  copyString(settings.username, sizeof(settings.username), preferences.getString(UserKey, ""));
  copyString(settings.password, sizeof(settings.password), preferences.getString(PassKey, ""));
  copyString(settings.telemetryTopic, sizeof(settings.telemetryTopic), preferences.getString(TopicKey, ""));

  preferences.end();
  return true;
}

bool save(const MqttSettings& settings) {
  Preferences preferences;
  if (!preferences.begin(Namespace, false)) {
    return false;
  }

  preferences.putString(HostKey, settings.host);
  preferences.putUShort(PortKey, settings.port);
  preferences.putString(UserKey, settings.username);
  preferences.putString(PassKey, settings.password);
  preferences.putString(TopicKey, settings.telemetryTopic);

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

}  // namespace MqttSettingsStore
