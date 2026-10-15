// Numele fișierelor de log: „log_0001.csv”. Pur, testabil pe PC.
// Pagina web acceptă doar nume care trec de isLogName (fără „/”, „..” etc.).
#pragma once

#include <cstdio>
#include <cstring>

// Întoarce indexul (1..9999) sau -1 dacă numele nu e un log valid.
// Acceptă și calea cu „/” în față.
inline int logIndex(const char* name) {
  if (!name) return -1;
  if (*name == '/') name++;
  if (std::strlen(name) != 12 || std::strncmp(name, "log_", 4) != 0 || std::strcmp(name + 8, ".csv") != 0)
    return -1;
  int v = 0;
  for (int i = 4; i < 8; i++) {
    if (name[i] < '0' || name[i] > '9') return -1;
    v = v * 10 + (name[i] - '0');
  }
  return v >= 1 ? v : -1;
}

inline bool isLogName(const char* name) { return name && *name != '/' && logIndex(name) > 0; }

inline void logName(int index, char* out, size_t cap) { std::snprintf(out, cap, "/log_%04d.csv", index); }
