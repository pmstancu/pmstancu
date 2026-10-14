"""aiohttp server: serves the dashboard and pushes frames to browsers over WebSocket."""

from __future__ import annotations

import asyncio
import json
import logging
import time
from pathlib import Path
from typing import Callable

from aiohttp import WSMsgType, web

from .logger import CsvLogger
from .protocol import Frame

log = logging.getLogger(__name__)
STATIC = Path(__file__).parent / "static"


class Hub:
    """Holds the latest frame and fans it out to connected dashboards."""

    def __init__(self, logger: CsvLogger | None, push_hz: float = 10.0):
        self.logger = logger
        self.push_period = 1.0 / push_hz
        self.latest: Frame | None = None
        self.latest_at = 0.0
        self.firmware_getter: Callable[[], str | None] = lambda: None
        self.clients: set[web.WebSocketResponse] = set()
        self._loop: asyncio.AbstractEventLoop | None = None

    def bind_loop(self, loop: asyncio.AbstractEventLoop) -> None:
        self._loop = loop

    def on_frame(self, frame: Frame) -> None:
        """Called from the source thread."""
        if self._loop:
            self._loop.call_soon_threadsafe(self._accept, frame)

    def _accept(self, frame: Frame) -> None:
        self.latest = frame
        self.latest_at = time.monotonic()
        if self.logger:
            try:
                self.logger.write(frame)
            except OSError as e:  # full disk etc. must not stop the display
                log.error("logging failed: %s", e)

    def status(self) -> dict:
        age = time.monotonic() - self.latest_at if self.latest else None
        return {
            "frame": self.latest.to_dict() if self.latest else None,
            "age_s": age,
            "stale": age is None or age > 1.0,
            "fw": self.firmware_getter(),
        }

    async def broadcast_loop(self) -> None:
        while True:
            await asyncio.sleep(self.push_period)
            if not self.clients:
                continue
            msg = json.dumps(self.status())
            for ws in list(self.clients):
                try:
                    await ws.send_str(msg)
                except (ConnectionResetError, RuntimeError):
                    self.clients.discard(ws)


def make_app(hub: Hub) -> web.Application:
    async def index(_request: web.Request) -> web.FileResponse:
        return web.FileResponse(STATIC / "index.html")

    async def latest(_request: web.Request) -> web.Response:
        return web.json_response(hub.status())

    async def ws_handler(request: web.Request) -> web.WebSocketResponse:
        ws = web.WebSocketResponse(heartbeat=10)
        await ws.prepare(request)
        hub.clients.add(ws)
        try:
            async for msg in ws:
                if msg.type == WSMsgType.ERROR:
                    break
        finally:
            hub.clients.discard(ws)
        return ws

    async def on_startup(app: web.Application) -> None:
        hub.bind_loop(asyncio.get_running_loop())
        app["broadcaster"] = asyncio.create_task(hub.broadcast_loop())

    async def on_cleanup(app: web.Application) -> None:
        app["broadcaster"].cancel()
        if hub.logger:
            hub.logger.close()

    app = web.Application()
    app.router.add_get("/", index)
    app.router.add_get("/api/latest", latest)
    app.router.add_get("/ws", ws_handler)
    app.router.add_static("/static/", STATIC)
    app.on_startup.append(on_startup)
    app.on_cleanup.append(on_cleanup)
    return app
