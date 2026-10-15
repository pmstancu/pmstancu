#include "logger.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "logname.h"
#include "pins.h"

namespace logger {
namespace {

SPIClass sdSpi(FSPI);
bool mounted = false;
bool writeOk = false;
File file;
char fileName[20] = "";
uint32_t lastRow = 0;
uint32_t lastFlush = 0;
uint32_t lastMountTry = 0;

constexpr uint32_t kFlushMs = 1000;
constexpr uint32_t kRetryMs = 10000;
constexpr uint32_t kSdHz = 20000000;

bool mount() {
  lastMountTry = millis();
  SD.end();
  mounted = SD.begin(PIN_SD_CS, sdSpi, kSdHz);
  return mounted;
}

int nextIndex() {
  int maxIdx = 0;
  File root = SD.open("/");
  if (!root) return 1;
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    int i = logIndex(f.name());
    if (i > maxIdx) maxIdx = i;
    f.close();
  }
  root.close();
  return maxIdx < 9999 ? maxIdx + 1 : 9999;
}

bool openNew() {
  logName(nextIndex(), fileName, sizeof fileName);
  file = SD.open(fileName, FILE_WRITE);
  if (!file) {
    fileName[0] = 0;
    return false;
  }
  char head[128];
  csvHeader(head, sizeof head);
  file.print(head);
  file.flush();
  return true;
}

void fail() {
  if (file) file.close();
  fileName[0] = 0;
  writeOk = false;
  mounted = false;
}

}  // namespace

void begin() {
  sdSpi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  writeOk = mount() && openNew();
}

void tick(const Snapshot& snap, const Settings& s) {
  uint32_t now = millis();
  if (!s.logEnabled) {
    if (file) {
      file.close();
      fileName[0] = 0;
    }
    return;
  }
  if (!file) {
    // Card scos sau eroare: reîncearcă periodic.
    if (now - lastMountTry >= kRetryMs) writeOk = (mounted || mount()) && openNew();
    if (!file) return;
  }
  if (now - lastRow < s.logPeriodMs) return;
  lastRow = now;

  char line[160];
  int n = csvRow(snap, line, sizeof line);
  if (n <= 0 || static_cast<size_t>(n) >= sizeof line || file.write(reinterpret_cast<uint8_t*>(line), n) != static_cast<size_t>(n)) {
    fail();
    return;
  }
  writeOk = true;
  // Flush rar: fiecare flush scrie în FAT; la tăierea curentului se pierde
  // cel mult ultima secundă.
  if (now - lastFlush >= kFlushMs) {
    file.flush();
    lastFlush = now;
  }
}

bool ok() { return writeOk; }

const char* currentFile() { return fileName; }

}  // namespace logger
