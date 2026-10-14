"""Parsing of the JSON-lines telemetry sent by the ESP32 over USB serial.

Frame format (one object per line):
    {"t": 12345, "v": {"ect": 88.1, ...}, "l": {"ect": 0, ...}, "f": {"ect": 0, ...}}
    {"hello": "esp32-gauges", "fw": "0.1.0"}

The channel keys must match kChannelKeys in firmware/esp32-gauges/include/readings.h.
"""

from __future__ import annotations

import json
import math
from dataclasses import dataclass, field

CHANNELS = ("ect", "eot", "iat", "egt", "boost", "afr", "vbat")

LEVEL_OK, LEVEL_WARN, LEVEL_CRIT = 0, 1, 2


@dataclass
class Frame:
    device_ms: int
    values: dict[str, float | None]
    levels: dict[str, int] = field(default_factory=dict)
    faults: dict[str, int] = field(default_factory=dict)

    def to_dict(self) -> dict:
        return {"t": self.device_ms, "v": self.values, "l": self.levels, "f": self.faults}


@dataclass
class Hello:
    device: str
    firmware: str


def _num_or_none(x) -> float | None:
    if isinstance(x, bool) or not isinstance(x, (int, float)):
        return None
    return None if math.isnan(x) or math.isinf(x) else float(x)


def parse_line(line: str | bytes) -> Frame | Hello | None:
    """Returns a Frame/Hello, or None for anything malformed (noise, boot logs)."""
    if isinstance(line, bytes):
        line = line.decode("utf-8", errors="replace")
    line = line.strip()
    if not line.startswith("{"):
        return None
    try:
        obj = json.loads(line)
    except json.JSONDecodeError:
        return None
    if not isinstance(obj, dict):
        return None

    if "hello" in obj:
        return Hello(device=str(obj["hello"]), firmware=str(obj.get("fw", "?")))

    raw_values = obj.get("v")
    if not isinstance(raw_values, dict) or not isinstance(obj.get("t"), int):
        return None
    levels = obj.get("l") if isinstance(obj.get("l"), dict) else {}
    faults = obj.get("f") if isinstance(obj.get("f"), dict) else {}
    return Frame(
        device_ms=obj["t"],
        values={ch: _num_or_none(raw_values.get(ch)) for ch in CHANNELS},
        levels={ch: int(levels.get(ch, LEVEL_OK)) for ch in CHANNELS},
        faults={ch: int(faults.get(ch, 0)) for ch in CHANNELS},
    )
