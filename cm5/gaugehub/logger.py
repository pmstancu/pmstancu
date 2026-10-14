"""CSV data logger: one file per day, flushed regularly so a power cut loses little."""

from __future__ import annotations

import csv
import datetime as dt
import time
from pathlib import Path
from typing import TextIO

from .protocol import CHANNELS, Frame


class CsvLogger:
    HEADER = ["time_iso", "device_ms", *CHANNELS, *(f"{c}_lvl" for c in CHANNELS), *(f"{c}_flt" for c in CHANNELS)]

    def __init__(self, directory: Path, flush_every_s: float = 2.0, min_interval_s: float = 0.1):
        self.directory = Path(directory)
        self.directory.mkdir(parents=True, exist_ok=True)
        self.flush_every_s = flush_every_s
        self.min_interval_s = min_interval_s
        self._file: TextIO | None = None
        self._writer = None
        self._day: dt.date | None = None
        self._last_flush = 0.0
        self._last_write = 0.0

    def _open_for(self, now: dt.datetime) -> None:
        self.close()
        path = self.directory / f"{now:%Y-%m-%d}.csv"
        new = not path.exists()
        self._file = path.open("a", newline="", encoding="utf-8")
        self._writer = csv.writer(self._file)
        if new:
            self._writer.writerow(self.HEADER)
        self._day = now.date()

    def write(self, frame: Frame, now: dt.datetime | None = None) -> bool:
        mono = time.monotonic()
        if mono - self._last_write < self.min_interval_s:
            return False
        self._last_write = mono
        now = now or dt.datetime.now()
        if self._day != now.date():
            self._open_for(now)

        def fmt(x):
            return "" if x is None else f"{x:.3f}"

        self._writer.writerow(
            [
                now.isoformat(timespec="milliseconds"),
                frame.device_ms,
                *(fmt(frame.values.get(c)) for c in CHANNELS),
                *(frame.levels.get(c, 0) for c in CHANNELS),
                *(frame.faults.get(c, 0) for c in CHANNELS),
            ]
        )
        if mono - self._last_flush >= self.flush_every_s:
            self._file.flush()
            self._last_flush = mono
        return True

    def close(self) -> None:
        if self._file:
            self._file.close()
            self._file = None
