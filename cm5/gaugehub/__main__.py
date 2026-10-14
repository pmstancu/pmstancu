"""Entry point: python -m gaugehub --port /dev/ttyACM0   (or --sim for bench testing)."""

from __future__ import annotations

import argparse
import logging
from pathlib import Path

from aiohttp import web

from .logger import CsvLogger
from .server import Hub, make_app
from .sources import SerialSource, SimSource


def main() -> None:
    p = argparse.ArgumentParser(description="1HD-FTE gauge hub (CM5 side)")
    p.add_argument("--port", default="/dev/ttyACM0", help="ESP32 USB serial port")
    p.add_argument("--sim", action="store_true", help="use simulated data instead of the ESP32")
    p.add_argument("--log-dir", type=Path, default=Path.home() / "gaugehub-logs")
    p.add_argument("--no-log", action="store_true", help="disable CSV logging")
    p.add_argument("--host", default="127.0.0.1", help="use 0.0.0.0 to view from a phone on the same Wi-Fi")
    p.add_argument("--http-port", type=int, default=8080)
    args = p.parse_args()

    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s: %(message)s")

    hub = Hub(logger=None if args.no_log else CsvLogger(args.log_dir))
    source = SimSource(hub.on_frame) if args.sim else SerialSource(args.port, hub.on_frame)

    hub.firmware_getter = lambda: source.firmware
    app = make_app(hub)

    async def start_source(_app: web.Application) -> None:
        source.start()

    async def stop_source(_app: web.Application) -> None:
        source.stop()

    app.on_startup.append(start_source)
    app.on_cleanup.append(stop_source)
    web.run_app(app, host=args.host, port=args.http_port)


if __name__ == "__main__":
    main()
