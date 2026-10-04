#include "app_state.h"
#include <cstring>

SystemStatus g_systemStatus;
SensorValues g_sensorValues;
LoggingStatus g_loggingStatus;
WeatherStatus g_weatherStatus;
MotionStatus g_motionStatus;
LogView g_logView;

void LoggingStatus::pushEvent(const char *timeTextIn, const char *textIn)
{
  // En yeni olay basa eklenir, en eski olay disari kayar (kaydirilabilir liste).
  for (int i = LOG_EVENT_COUNT - 1; i > 0; i--) {
    events[i] = events[i - 1];
  }
  strncpy(events[0].timeText, timeTextIn, sizeof(events[0].timeText) - 1);
  events[0].timeText[sizeof(events[0].timeText) - 1] = '\0';
  strncpy(events[0].text, textIn, sizeof(events[0].text) - 1);
  events[0].text[sizeof(events[0].text) - 1] = '\0';

  if (eventCount < LOG_EVENT_COUNT) {
    eventCount++;
  }
  eventsVersion++;
}
