"""Data sources: the ESP32 over USB serial, or a simulator for bench testing."""

from __future__ import annotations

import logging
import math
import random
import threading
import time
from typing import Callable

from .protocol import CHANNELS, LEVEL_CRIT, LEVEL_OK, LEVEL_WARN, Frame, Hello, parse_line

log = logging.getLogger(__name__)

FrameCallback = Callable[[Frame], None]


class SerialSource:
    """Reads JSON lines from the ESP32 in a background thread, reconnecting forever."""

    def __init__(self, port: str, on_frame: FrameCallback, baud: int = 115200):
        self.port = port
        self.baud = baud
        self.on_frame = on_frame
        self.firmware: str | None = None
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, name="serial", daemon=True)

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()

    def _run(self) -> None:
        import serial  # pyserial; imported here so the simulator works without it

        while not self._stop.is_set():
            try:
                with serial.Serial(self.port, self.baud, timeout=1) as ser:
                    log.info("connected to %s", self.port)
                    while not self._stop.is_set():
                        line = ser.readline()
                        if not line:
                            continue
                        msg = parse_line(line)
                        if isinstance(msg, Frame):
                            self.on_frame(msg)
                        elif isinstance(msg, Hello):
                            if msg.firmware != self.firmware:
                                log.info("device %s firmware %s", msg.device, msg.firmware)
                            self.firmware = msg.firmware
            except (serial.SerialException, OSError) as e:
                log.warning("serial %s unavailable (%s); retrying in 2 s", self.port, e)
                self._stop.wait(2)


class SimSource:
    """Plausible fake data (warm-up, cruise, and periodic full-load pulls) for UI work."""

    def __init__(self, on_frame: FrameCallback, hz: float = 10.0):
        self.on_frame = on_frame
        self.period = 1.0 / hz
        self.firmware = "simulator"
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, name="sim", daemon=True)

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()

    @staticmethod
    def _level(value: float, warn: float, crit: float) -> int:
        return LEVEL_CRIT if value >= crit else LEVEL_WARN if value >= warn else LEVEL_OK

    def sample(self, t: float) -> Frame:
        warm = min(1.0, t / 120.0)
        load = max(0.0, math.sin(t / 7.0)) ** 4  # occasional pulls
        v = {
            "ect": 20 + 68 * warm + 4 * load,
            "eot": 20 + 75 * warm + 8 * load,
            "iat": 25 + 25 * load,
            "egt": 250 + 450 * load + random.uniform(-5, 5),
            "boost": 1.52 * load + random.uniform(-0.01, 0.01),
            "afr": 14.7 * (2.5 - 1.3 * load),
            "vbat": 14.1 + random.uniform(-0.05, 0.05),
        }
        levels = {ch: LEVEL_OK for ch in CHANNELS}
        levels["egt"] = self._level(v["egt"], 650, 720)
        levels["boost"] = self._level(v["boost"], 1.59, 1.72)
        return Frame(device_ms=int(t * 1000), values=v, levels=levels, faults={ch: 0 for ch in CHANNELS})

    def _run(self) -> None:
        t0 = time.monotonic()
        while not self._stop.wait(self.period):
            self.on_frame(self.sample(time.monotonic() - t0))
