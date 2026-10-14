// Dashboard client. Alarm levels come from the ESP32 (single source of truth);
// the warn/crit values here only position the tick marks on the meters, so keep
// them in sync with firmware/esp32-gauges/include/config.h.
"use strict";

const PSI_PER_BAR = 14.5038;
const BOOST_UNIT = "psi"; // "psi" or "bar"
const AFR_STOICH_GAS = 14.7;

const CHANNELS = {
  egt:   { min: 0,   max: 900, warn: 650, crit: 720, dec: 0 },
  boost: { min: 0,   max: BOOST_UNIT === "psi" ? 30 : 2.0, warn: 1.59, crit: 1.72, dec: 1, toDisplay: boostDisplay },
  afr:   { min: 1.0, max: 3.0, dec: 2, toDisplay: (afr) => afr / AFR_STOICH_GAS },
  ect:   { min: 40,  max: 120, warn: 100, crit: 105, dec: 0 },
  eot:   { min: 40,  max: 140, warn: 120, crit: 130, dec: 0 },
  iat:   { min: 0,   max: 90,  warn: 60,  crit: 70,  dec: 0 },
  vbat:  { min: 10,  max: 16,  dec: 1 },
};

const LEVEL_TEXT = { 1: "⚠ AVERTIZARE", 2: "⛔ CRITIC" };
const TREND_SECONDS = 60;
const egtHistory = [];

function boostDisplay(bar) { return BOOST_UNIT === "psi" ? bar * PSI_PER_BAR : bar; }

const clamp01 = (x) => Math.max(0, Math.min(1, x));

function displayValue(ch, raw) {
  const cfg = CHANNELS[ch];
  return cfg.toDisplay ? cfg.toDisplay(raw) : raw;
}

function frac(ch, raw) {
  const cfg = CHANNELS[ch];
  return clamp01((displayValue(ch, raw) - cfg.min) / (cfg.max - cfg.min));
}

// --- static setup: unit labels and threshold ticks
function setupTicks() {
  document.querySelector('[data-ch="boost"] .unit').textContent = BOOST_UNIT;
  for (const [ch, cfg] of Object.entries(CHANNELS)) {
    const tile = document.querySelector(`[data-ch="${ch}"]`);
    for (const kind of ["warn", "crit"]) {
      if (cfg[kind] === undefined) continue;
      const f = frac(ch, cfg[kind]);
      const meterTick = tile.querySelector(`.meter .tick.${kind}`);
      if (meterTick) meterTick.style.left = `calc(${f * 100}% - 1px)`;
      const arcTick = tile.querySelector(`.arc-tick.${kind}`);
      if (arcTick) {
        const a = ((135 + f * 270) * Math.PI) / 180;
        arcTick.setAttribute("x1", 100 + 58 * Math.cos(a));
        arcTick.setAttribute("y1", 90 + 58 * Math.sin(a));
        arcTick.setAttribute("x2", 100 + 80 * Math.cos(a));
        arcTick.setAttribute("y2", 90 + 80 * Math.sin(a));
      }
    }
  }
  const fill = document.querySelector(".arc-fill");
  fill.style.strokeDasharray = "100 100";
  fill.style.strokeDashoffset = "100";
}

function renderTile(ch, raw, level, fault) {
  const tile = document.querySelector(`[data-ch="${ch}"]`);
  const cfg = CHANNELS[ch];
  const ok = raw !== null && raw !== undefined;
  const status = tile.querySelector(".status");

  tile.classList.toggle("warn", ok && level === 1);
  tile.classList.toggle("crit", ok && level === 2);
  tile.classList.toggle("fault", !ok);

  // "+ 0" turns -0 into 0 so a value hovering at zero never shows "-0.0".
  tile.querySelector(".num").textContent = ok ? (Number(displayValue(ch, raw).toFixed(cfg.dec)) + 0).toFixed(cfg.dec) : "---";
  if (!ok) {
    status.hidden = false;
    status.textContent = fault === 255 ? "SENZOR LIPSĂ" : `DEFECT 0x${fault.toString(16).toUpperCase()}`;
  } else if (level >= 1) {
    status.hidden = false;
    status.textContent = LEVEL_TEXT[level];
  } else {
    status.hidden = true;
  }

  const f = ok ? frac(ch, raw) : 0;
  const meter = tile.querySelector(".meter-fill");
  if (meter) meter.style.width = `${f * 100}%`;
  const arc = tile.querySelector(".arc-fill");
  if (arc) arc.style.strokeDashoffset = String(100 - f * 100);

  if (ch === "afr") tile.querySelector(".afr").textContent = ok ? raw.toFixed(1) : "---";
}

function drawTrend(nowMs) {
  const canvas = document.querySelector(".trend canvas");
  const ctx = canvas.getContext("2d");
  const w = canvas.width, h = canvas.height, cfg = CHANNELS.egt;
  const css = getComputedStyle(document.documentElement);
  ctx.clearRect(0, 0, w, h);

  const y = (v) => h - 4 - clamp01((v - cfg.min) / (cfg.max - cfg.min)) * (h - 8);
  ctx.lineWidth = 1;
  ctx.setLineDash([4, 4]);
  for (const [kind, color] of [["warn", "--ink-3"], ["crit", "--ink-2"]]) {
    ctx.strokeStyle = css.getPropertyValue(color);
    ctx.beginPath(); ctx.moveTo(0, y(cfg[kind])); ctx.lineTo(w, y(cfg[kind])); ctx.stroke();
  }
  ctx.setLineDash([]);

  ctx.strokeStyle = css.getPropertyValue("--accent");
  ctx.lineWidth = 2;
  ctx.lineJoin = "round";
  ctx.beginPath();
  let pen = false;
  for (const [t, v] of egtHistory) {
    const x = w - ((nowMs - t) / (TREND_SECONDS * 1000)) * w;
    if (v === null) { pen = false; continue; }  // gap on sensor fault
    pen ? ctx.lineTo(x, y(v)) : ctx.moveTo(x, y(v));
    pen = true;
  }
  ctx.stroke();
}

function render(status) {
  const link = document.getElementById("link");
  document.body.classList.toggle("stale", status.stale);
  link.className = "link " + (status.stale ? "bad" : "ok");
  link.textContent = status.stale
    ? (status.frame ? "fără date de la ESP32" : "aștept ESP32…")
    : `ESP32 fw ${status.fw ?? "?"}`;
  if (!status.frame) return;

  const { v, l, f } = status.frame;
  for (const ch of Object.keys(CHANNELS)) renderTile(ch, v[ch], l[ch] ?? 0, f[ch] ?? 0);

  const now = Date.now();
  egtHistory.push([now, status.stale ? null : v.egt]);
  while (egtHistory.length && now - egtHistory[0][0] > TREND_SECONDS * 1000) egtHistory.shift();
  drawTrend(now);
}

function connect() {
  const ws = new WebSocket(`${location.protocol === "https:" ? "wss" : "ws"}://${location.host}/ws`);
  ws.onmessage = (ev) => render(JSON.parse(ev.data));
  ws.onclose = () => {
    render({ stale: true, frame: null, fw: null });
    setTimeout(connect, 1000);
  };
}

setupTicks();
connect();
