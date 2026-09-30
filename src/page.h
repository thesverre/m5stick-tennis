#pragma once

// Web UI served at "/". Receives IMU samples over the /ws WebSocket and draws live charts
// plus a 3D model of the stick that follows its orientation.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>M5StickC Motion</title>
<style>
:root {
  color-scheme: light;
  --page: #f9f9f7; --surface: #fcfcfb; --ink: #0b0b0b; --ink-2: #52514e; --muted: #898781;
  --grid: #e1e0d9; --axis: #c3c2b7; --border: rgba(11,11,11,0.10);
  --x: #2a78d6; --y: #eb6834; --z: #1baf7a; --good: #0ca30c; --bad: #d03b3b;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    color-scheme: dark;
    --page: #0d0d0d; --surface: #1a1a19; --ink: #fff; --ink-2: #c3c2b7; --muted: #898781;
    --grid: #2c2c2a; --axis: #383835; --border: rgba(255,255,255,0.10);
    --x: #3987e5; --y: #d95926; --z: #199e70;
  }
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--page); color: var(--ink);
  font: 14px/1.4 system-ui, -apple-system, "Segoe UI", sans-serif; }
main { max-width: 960px; margin: 0 auto; padding: 20px 16px 40px; }
header { display: flex; flex-wrap: wrap; align-items: baseline; gap: 8px 16px; margin-bottom: 16px; }
h1 { font-size: 20px; margin: 0; }
.status { color: var(--ink-2); display: flex; align-items: center; gap: 6px; }
.dot { width: 8px; height: 8px; border-radius: 50%; background: var(--muted); }
.dot.ok { background: var(--good); } .dot.err { background: var(--bad); }
.navlink { margin-left: auto; color: var(--x); text-decoration: none; font-weight: 600; }
header button { font: inherit; padding: 6px 14px; border-radius: 8px;
  border: 1px solid var(--border); background: var(--surface); color: var(--ink); cursor: pointer; }
.card { background: var(--surface); border: 1px solid var(--border); border-radius: 12px;
  padding: 16px; margin-bottom: 16px; }
.card h2 { font-size: 15px; margin: 0; }
.card .unit { color: var(--muted); font-weight: normal; }
.top { display: flex; flex-wrap: wrap; justify-content: space-between; gap: 8px; margin-bottom: 10px; }
.legend { display: flex; gap: 14px; color: var(--ink-2); }
.legend span::before { content: ""; display: inline-block; width: 12px; height: 3px; border-radius: 2px;
  margin-right: 6px; vertical-align: middle; background: var(--c); }
.tiles { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-bottom: 10px; }
.tile { border-left: 3px solid var(--c); padding: 2px 10px; }
.tile .k { color: var(--muted); font-size: 12px; }
.tile .v { font-size: 22px; font-variant-numeric: tabular-nums; }
.plot { position: relative; height: 220px; }
canvas { width: 100%; height: 100%; display: block; touch-action: none; }
.tip { position: absolute; pointer-events: none; background: var(--surface); border: 1px solid var(--border);
  border-radius: 8px; padding: 6px 10px; font-size: 12px; box-shadow: 0 2px 8px rgba(0,0,0,.12);
  font-variant-numeric: tabular-nums; display: none; white-space: nowrap; }
.tip b { display: inline-block; width: 10px; height: 3px; border-radius: 2px; margin-right: 6px; vertical-align: middle; }
footer { color: var(--muted); font-size: 12px; }
.card .top button { font: inherit; font-size: 13px; padding: 4px 12px; border-radius: 8px;
  border: 1px solid var(--border); background: var(--surface); color: var(--ink); cursor: pointer; }

/* 3D stick: CSS box built from six faces, 5 px per mm (24 x 48 x 14 mm) */
.stage { position: relative; height: 420px; perspective: 1000px; overflow: hidden;
  display: flex; align-items: center; justify-content: center; }
.world { position: relative; width: 0; height: 0; transform-style: preserve-3d; transform: rotateX(50deg) scale3d(.75, .75, .75); }
.floor { position: absolute; width: 360px; height: 360px; left: -180px; top: -180px; border-radius: 50%;
  transform: translateZ(-90px);
  background: radial-gradient(closest-side, rgba(0,0,0,.28), rgba(0,0,0,0)); }
.floor::after { content: ""; position: absolute; inset: 0; border-radius: 50%;
  background: repeating-radial-gradient(circle, transparent 0 59px, var(--grid) 59px 60px); opacity: .8; }
.stick { position: absolute; transform-style: preserve-3d; --o: #f47a20; --o2: #d9621a; --o3: #bd5314; }
.face { position: absolute; left: 50%; top: 50%; backface-visibility: hidden; border-radius: 3px; }
.f-front, .f-back { width: 120px; height: 240px; margin: -120px 0 0 -60px; border-radius: 6px; }
.f-left, .f-right { width: 70px; height: 240px; margin: -120px 0 0 -35px; }
.f-top, .f-bottom { width: 120px; height: 70px; margin: -35px 0 0 -60px; }
.f-front { background: var(--o); transform: translateZ(35px); box-shadow: inset 0 0 0 2px var(--o2); }
.f-back { background: var(--o2); transform: rotateY(180deg) translateZ(35px);
  display: flex; align-items: center; justify-content: center; }
.f-back span { transform: rotate(-90deg); color: rgba(0,0,0,.45); font: 700 20px system-ui, sans-serif; letter-spacing: .5px; }
.f-right { background: var(--o3); transform: rotateY(90deg) translateZ(60px); }
.f-left { background: var(--o3); transform: rotateY(-90deg) translateZ(60px); }
.f-top { background: var(--o2); transform: rotateX(90deg) translateZ(120px); }
.f-bottom { background: var(--o2); transform: rotateX(-90deg) translateZ(120px); }
.screen { position: absolute; left: 14px; right: 14px; top: 16px; height: 128px; border-radius: 6px;
  background: #111; padding: 10px 8px; }
.lcd { width: 100%; height: 100%; background: #000; overflow: hidden; position: relative; }
.lcd div { position: absolute; left: 50%; top: 50%; width: 108px; transform: translate(-50%, -50%) rotate(90deg);
  text-align: center; font: 700 11px/1.5 system-ui, sans-serif; color: #fff; }
.lcd b { color: #2ecc40; }
.m5btn { position: absolute; left: 30px; right: 30px; top: 166px; height: 46px; border-radius: 10px;
  background: var(--o2); box-shadow: inset 0 -3px 0 rgba(0,0,0,.18);
  display: flex; align-items: center; justify-content: center; color: rgba(255,255,255,.9);
  font: 800 16px system-ui, sans-serif; }
.sidebtn { position: absolute; left: 22px; width: 26px; height: 40px; border-radius: 4px; background: var(--o2);
  box-shadow: inset 0 0 0 2px rgba(0,0,0,.15); }
.usb { position: absolute; left: 50%; top: 50%; width: 46px; height: 16px; margin: -8px 0 0 -23px;
  border-radius: 8px; background: #222; box-shadow: inset 0 0 0 3px #555; }
.pins { position: absolute; inset: 22px 12px; display: flex; justify-content: space-between; align-items: center; }
.pins i { width: 7px; height: 7px; border-radius: 50%; background: #3a2410; }
.shadow { position: absolute; width: 180px; height: 260px; left: -90px; top: -130px; border-radius: 50%;
  background: radial-gradient(closest-side, rgba(0,0,0,.45), rgba(0,0,0,0)); }
.toggle { display: flex; align-items: center; gap: 6px; color: var(--ink-2); cursor: pointer; }
.angles { display: flex; gap: 20px; color: var(--ink-2); font-variant-numeric: tabular-nums; }
.angles b { color: var(--ink); font-weight: 600; }
.hint { color: var(--muted); font-size: 12px; margin-top: 4px; }

/* Air mouse: a virtual 1000 x 600 screen the stick points at */
.pad { position: relative; aspect-ratio: 5 / 3; border-radius: 10px; overflow: hidden;
  border: 1px solid var(--border); background-color: var(--page);
  background-image: radial-gradient(circle, var(--grid) 1.2px, transparent 1.4px); background-size: 24px 24px; }
.mouse-ctl { display: flex; flex-wrap: wrap; align-items: center; gap: 8px 16px; color: var(--ink-2); }
.mouse-ctl b { color: var(--ink); font-weight: 600; font-variant-numeric: tabular-nums; }
.mouse-ctl input[type=range] { width: 110px; vertical-align: middle; }
.chip { padding: 2px 10px; border-radius: 999px; border: 1px solid var(--border); color: var(--muted); font-size: 12px; }
.chip.on { background: var(--y); border-color: var(--y); color: #fff; }
</style>
</head>
<body>
<main>
  <header>
    <h1>M5StickC motion</h1>
    <div class="status"><span class="dot" id="dot"></span><span id="status">Connecting…</span></div>
    <a class="navlink" href="/tennis">Tennis game →</a>
    <button id="pause">Pause</button>
  </header>

  <section class="card" id="airmouse">
    <div class="top"><h2>Air mouse</h2>
      <div class="mouse-ctl">
        <span class="chip" id="chip-a">M5 button</span><span class="chip" id="chip-b">Side button</span>
        <span>Hits <b id="hits">0</b></span>
        <label>Speed <input type="range" id="sens" min="0.4" max="2.5" step="0.1" value="1"></label>
        <button id="center">Center</button>
      </div></div>
    <div class="pad"><canvas id="pad"></canvas></div>
    <div class="hint">Hold the stick like a remote: screen up, top end pointing at your screen. Turn it left/right and
      up/down to move the pointer. M5 button clicks (hold it to draw); the side button re-centers the pointer.</div>
  </section>

  <section class="card" id="orient">
    <div class="top"><h2>Orientation</h2>
      <div class="angles"><span>Roll <b id="roll">–</b></span><span>Pitch <b id="pitch">–</b></span><span>Yaw <b id="yaw">–</b></span></div>
      <label class="toggle"><input type="checkbox" id="move" checked> Follow movement</label>
      <button id="recenter">Recenter</button></div>
    <div class="stage">
      <div class="world">
        <div class="floor"></div>
        <div class="shadow" id="shadow"></div>
        <div class="stick" id="stick">
          <div class="face f-front">
            <div class="screen"><div class="lcd"><div><b>m5stick.local</b><br><span id="lcd-ip"></span></div></div></div>
            <div class="m5btn">M5</div>
          </div>
          <div class="face f-back"><span>M5StickC</span></div>
          <div class="face f-right"><div class="sidebtn" style="top:40px"></div></div>
          <div class="face f-left"><div class="sidebtn" style="top:60px"></div></div>
          <div class="face f-top"><div class="pins"><i></i><i></i><i></i><i></i><i></i><i></i><i></i><i></i></div></div>
          <div class="face f-bottom"><div class="usb"></div></div>
        </div>
      </div>
    </div>
    <div class="hint">Tilt is absolute (from gravity). Heading comes from the gyro and slowly drifts — press Recenter to reset it.
      Movement is estimated from acceleration, so the model follows quick moves and then eases back to the center.</div>
  </section>

  <section class="card" id="gyro">
    <div class="top"><h2>Gyroscope <span class="unit">°/s</span></h2>
      <div class="legend"><span style="--c:var(--x)">X</span><span style="--c:var(--y)">Y</span><span style="--c:var(--z)">Z</span></div></div>
    <div class="tiles"></div>
    <div class="plot"><canvas></canvas><div class="tip"></div></div>
  </section>

  <section class="card" id="accel">
    <div class="top"><h2>Accelerometer <span class="unit">g</span></h2>
      <div class="legend"><span style="--c:var(--x)">X</span><span style="--c:var(--y)">Y</span><span style="--c:var(--z)">Z</span></div></div>
    <div class="tiles"></div>
    <div class="plot"><canvas></canvas><div class="tip"></div></div>
  </section>

  <footer>Streamed over WebSocket at 100 Hz, last 10 seconds shown. Hover a chart to read values.</footer>
</main>
<script src="/motion.js"></script>
<script>
const WINDOW_MS = 10000;
const AXES = ["x", "y", "z"];
const samples = [];          // {t, g:[x,y,z], a:[x,y,z]}
let paused = false, imu = "", frameQueued = false;

const css = n => getComputedStyle(document.documentElement).getPropertyValue(n).trim();

function makeChart(id, key, unit, digits, minRange) {
  const root = document.getElementById(id);
  const canvas = root.querySelector("canvas"), tip = root.querySelector(".tip");
  const tiles = root.querySelector(".tiles");
  tiles.innerHTML = AXES.map(a =>
    `<div class="tile" style="--c:var(--${a})"><div class="k">${a.toUpperCase()}</div><div class="v">–</div></div>`).join("");
  const vals = tiles.querySelectorAll(".v");
  const chart = { key, unit, digits, minRange, canvas, tip, vals, hoverX: null };
  canvas.addEventListener("pointermove", e => { chart.hoverX = e.offsetX; draw(chart); });
  canvas.addEventListener("pointerleave", () => { chart.hoverX = null; tip.style.display = "none"; draw(chart); });
  return chart;
}

const charts = [
  makeChart("gyro", "g", "°/s", 1, 50),
  makeChart("accel", "a", "g", 2, 1.2),
];

function niceStep(range) {
  const raw = range / 4, p = Math.pow(10, Math.floor(Math.log10(raw))), f = raw / p;
  return (f < 1.5 ? 1 : f < 3 ? 2 : f < 7 ? 5 : 10) * p;
}

function draw(c) {
  const { canvas } = c, dpr = window.devicePixelRatio || 1;
  const W = canvas.clientWidth, H = canvas.clientHeight;
  if (canvas.width !== W * dpr || canvas.height !== H * dpr) { canvas.width = W * dpr; canvas.height = H * dpr; }
  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, W, H);

  const L = 44, R = 56, T = 8, B = 22, pw = W - L - R, ph = H - T - B;
  const now = samples.length ? samples[samples.length - 1].t : 0;
  const vis = samples.filter(s => s.t >= now - WINDOW_MS);

  // Symmetric y-scale around zero, never narrower than minRange
  let m = c.minRange;
  for (const s of vis) for (const v of s[c.key]) m = Math.max(m, Math.abs(v));
  const step = niceStep(2 * m), top = Math.ceil(m / step) * step;
  const X = t => L + pw * (1 - (now - t) / WINDOW_MS);
  const Y = v => T + ph * (1 - (v + top) / (2 * top));

  // Grid + y ticks
  ctx.font = "11px system-ui, sans-serif"; ctx.textBaseline = "middle"; ctx.textAlign = "right";
  for (let v = -top; v <= top + 1e-9; v += step) {
    const y = Math.round(Y(v)) + 0.5;
    ctx.strokeStyle = Math.abs(v) < 1e-9 ? css("--axis") : css("--grid");
    ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(L, y); ctx.lineTo(L + pw, y); ctx.stroke();
    ctx.fillStyle = css("--muted"); ctx.fillText(+v.toFixed(c.digits), L - 6, y);
  }
  // x ticks: seconds ago
  ctx.textAlign = "center"; ctx.textBaseline = "top";
  for (let s = 0; s <= WINDOW_MS / 1000; s += 2) {
    ctx.fillText(s === 0 ? "now" : `-${s}s`, L + pw * (1 - s * 1000 / WINDOW_MS), T + ph + 6);
  }

  if (vis.length < 2) return;

  // Series lines + direct labels at the latest value
  const last = vis[vis.length - 1];
  const labelYs = [];
  AXES.forEach((a, i) => {
    ctx.strokeStyle = css(`--${a}`); ctx.lineWidth = 2; ctx.lineJoin = "round";
    ctx.beginPath();
    vis.forEach((s, j) => { const x = X(s.t), y = Y(s[c.key][i]); j ? ctx.lineTo(x, y) : ctx.moveTo(x, y); });
    ctx.stroke();
    labelYs.push({ a, y: Y(last[c.key][i]) });
  });
  // Nudge labels apart so they don't overlap
  labelYs.sort((p, q) => p.y - q.y);
  for (let i = 1; i < labelYs.length; i++) labelYs[i].y = Math.max(labelYs[i].y, labelYs[i - 1].y + 13);
  ctx.textAlign = "left"; ctx.textBaseline = "middle"; ctx.font = "600 11px system-ui, sans-serif";
  for (const { a, y } of labelYs) {
    ctx.fillStyle = css(`--${a}`); ctx.fillRect(L + pw + 6, y - 1.5, 8, 3);
    ctx.fillStyle = css("--ink-2"); ctx.fillText(a.toUpperCase(), L + pw + 18, y);
  }

  // Hover crosshair + tooltip
  if (c.hoverX !== null && c.hoverX >= L && c.hoverX <= L + pw) {
    const t = now - (1 - (c.hoverX - L) / pw) * WINDOW_MS;
    let best = vis[0];
    for (const s of vis) if (Math.abs(s.t - t) < Math.abs(best.t - t)) best = s;
    const x = Math.round(X(best.t)) + 0.5;
    ctx.strokeStyle = css("--muted"); ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(x, T); ctx.lineTo(x, T + ph); ctx.stroke();
    AXES.forEach((a, i) => {
      ctx.fillStyle = css(`--${a}`); ctx.strokeStyle = css("--surface"); ctx.lineWidth = 2;
      ctx.beginPath(); ctx.arc(x, Y(best[c.key][i]), 4, 0, 7); ctx.fill(); ctx.stroke();
    });
    c.tip.innerHTML = `<div style="color:var(--muted)">${((now - best.t) / 1000).toFixed(2)} s ago</div>` +
      AXES.map((a, i) => `<div><b style="background:var(--${a})"></b>${a.toUpperCase()} ${best[c.key][i].toFixed(c.digits)} ${c.unit}</div>`).join("");
    c.tip.style.display = "block";
    const tw = c.tip.offsetWidth;
    c.tip.style.left = (x + 12 + tw > W ? x - 12 - tw : x + 12) + "px";
    c.tip.style.top = T + "px";
  }
}

function render() {
  renderMouse();
  renderOrientation();
  const last = samples[samples.length - 1];
  for (const c of charts) {
    if (last) c.vals.forEach((el, i) => el.textContent = last[c.key][i].toFixed(c.digits));
    draw(c);
  }
}

function setStatus(ok, text) {
  document.getElementById("dot").className = "dot " + (ok ? "ok" : "err");
  document.getElementById("status").textContent = text;
}

// Position from acceleration, using zero-velocity updates (ZUPT):
//  - the acceleration offset is learned while the stick is at rest and subtracted,
//  - during a move, acceleration is integrated to velocity and displacement with no leak,
//  - when the stick comes to rest its true velocity is zero, so any velocity left over is
//    accumulated error; assuming it grew linearly over the move, it is removed from the
//    displacement after the fact (-v_end * T / 2), which fixes the direction of the move,
//  - at rest, the model then eases back to the center.
const G = 9.81, POS_LEAK = 0.8, DEADBAND = 0.02, LONG_MOVE_S = 1.5, LONG_MOVE_LEAK = 1.0;
const STILL_GYRO = 10, STILL_ACC = 0.04, STILL_AFTER_MS = 150;
const track = {
  vel: [0, 0, 0], pos: [0, 0, 0],  // world-frame m/s and m, from gravity-free acceleration
  moving: false, segT: 0, segStart: [0, 0, 0], segDisp: [0, 0, 0],  // current motion segment
  accBase: null,                    // that acceleration measured at rest = sensor offset
  stillMs: 0,                       // how long the stick has looked motionless
  reset() {
    this.vel = [0, 0, 0]; this.pos = [0, 0, 0]; this.moving = false; this.accBase = null; this.stillMs = 0;
  },
};

function updatePosition(s, gMag, dt) {
  const R = rotationMatrix(orient.q), a = s.a;
  const lin = [0, 1, 2].map(r => R[r][0] * a[0] + R[r][1] * a[1] + R[r][2] * a[2]);
  lin[2] -= 1;  // remove gravity (world Z = up)
  // Assume we start at rest: whatever |a| reads then beyond 1 g is sensor offset
  if (!track.accBase) track.accBase = [0, 0, Math.hypot(...a) - 1];

  // Motionless = not rotating AND the acceleration vector steady, for a little while.
  // Gyro alone isn't enough (sliding without turning barely moves it), and |a| alone isn't
  // either (a sideways push adds in quadrature to gravity, so |a| hardly changes). The
  // vector is compared to its own recent average, since this sensor reads ~2-3% high at rest.
  const quiet = gMag < STILL_GYRO && orient.aDev < STILL_ACC;
  track.stillMs = quiet ? track.stillMs + dt * 1000 : 0;
  const still = track.stillMs >= STILL_AFTER_MS;

  if (still) {
    for (let i = 0; i < 3; i++) track.accBase[i] += (lin[i] - track.accBase[i]) * Math.min(1, dt / 0.3);
    if (track.moving) {
      // Move finished: remove the velocity error from the displacement
      for (let i = 0; i < 3; i++) {
        track.pos[i] = track.segStart[i] + track.segDisp[i] - track.vel[i] * track.segT / 2;
      }
      track.vel = [0, 0, 0];
      track.moving = false;
    }
    for (let i = 0; i < 3; i++) track.pos[i] -= track.pos[i] * Math.min(1, POS_LEAK * dt);
    return;
  }

  if (!track.moving) {
    track.moving = true;
    track.segT = 0;
    track.segStart = [...track.pos];
    track.segDisp = [0, 0, 0];
    track.vel = [0, 0, 0];
  }
  track.segT += dt;
  for (let i = 0; i < 3; i++) {
    const d = lin[i] - track.accBase[i];
    track.vel[i] += Math.sign(d) * Math.max(0, Math.abs(d) - DEADBAND) * G * dt;
    // Continuous waving never comes to rest, so slowly bleed velocity on long moves
    if (track.segT > LONG_MOVE_S) track.vel[i] -= track.vel[i] * Math.min(1, LONG_MOVE_LEAK * dt);
    track.segDisp[i] += track.vel[i] * dt;
    track.pos[i] = track.segStart[i] + track.segDisp[i];
  }
}

function renderOrientation() {
  if (!orient.q) return;
  const R = rotationMatrix(orient.q);
  // CSS has y pointing down; flip the y axis on both sides (C·R·C, C = diag(1,-1,1))
  const c = [1, -1, 1], m = [];
  for (let col = 0; col < 3; col++) {
    for (let row = 0; row < 3; row++) m.push(c[row] * R[row][col] * c[col]);
    m.push(0);
  }
  m.push(0, 0, 0, 1);
  // 1 m of motion = 1200 px, softly limited (tanh) so the model slows near the edge of the
  // view instead of leaving it; CSS y is flipped like the rotation
  const PX = 1200, LIMIT = [170, 110, 110], follow = document.getElementById("move").checked;
  const [px, py, pz] = follow ? track.pos.map((v, i) => LIMIT[i] * Math.tanh(v * PX / LIMIT[i])) : [0, 0, 0];
  document.getElementById("stick").style.transform =
    `translate3d(${px.toFixed(1)}px, ${(-py).toFixed(1)}px, ${pz.toFixed(1)}px) matrix3d(${m.map(v => v.toFixed(5)).join(",")})`;
  // Shadow stays on the floor, follows x/y, and fades/shrinks as the stick lifts
  const lift = Math.max(0, Math.min(1, pz / 110));
  const shadow = document.getElementById("shadow");
  shadow.style.transform = `translate3d(${px.toFixed(1)}px, ${(-py).toFixed(1)}px, -90px) scale(${(1 - 0.4 * lift).toFixed(3)})`;
  shadow.style.opacity = (1 - 0.6 * lift).toFixed(3);

  const roll = Math.atan2(R[2][1], R[2][2]) / DEG;
  const pitch = Math.asin(Math.max(-1, Math.min(1, -R[2][0]))) / DEG;
  const yaw = Math.atan2(R[1][0], R[0][0]) / DEG;
  document.getElementById("roll").textContent = `${roll.toFixed(0)}°`;
  document.getElementById("pitch").textContent = `${pitch.toFixed(0)}°`;
  document.getElementById("yaw").textContent = `${yaw.toFixed(0)}°`;
}

document.getElementById("recenter").onclick = () => {
  const last = samples[samples.length - 1];
  orient.reset();
  track.reset();
  if (last) { orient.q = quatFromAccel(last.a); orient.lastT = last.t; renderOrientation(); }
};
document.getElementById("move").onchange = () => {
  track.vel = [0, 0, 0]; track.pos = [0, 0, 0];
  renderOrientation();
};
// ---- Air mouse: turning the stick moves a pointer across a virtual 1000 x 600 screen ----
// Like a gyro remote, the pointer follows rotation *rates* rather than absolute angles, so
// heading drift doesn't matter. Rates are taken about world axes (vertical for left/right,
// the horizontal axis across the stick for up/down), so it steers the same way however the
// stick is rolled in the hand.
const PAD_W = 1000, PAD_H = 600, TARGET_R = 36;
const JITTER_DPS = 3;         // rates around this and below (hand tremor, sensor noise) fade out
const CLICK_FREEZE_MS = 150;  // pressing the button nudges the stick, so hold the pointer meanwhile
const STROKE_FADE_MS = 2500, POP_MS = 400;
const mouse = {
  x: PAD_W / 2, y: PAD_H / 2, btn: 0, freezeUntil: 0, hits: 0, sens: 1,
  right: [1, 0, 0],  // last usable "right" axis, body frame
  targets: [], pops: [], strokes: [], stroke: null,
};

const dot3 = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
const overTarget = t => Math.hypot(t.x - mouse.x, t.y - mouse.y) <= TARGET_R + 8;

function spawnTarget() {
  for (let tries = 0; ; tries++) {
    const t = { x: TARGET_R + 20 + Math.random() * (PAD_W - 2 * TARGET_R - 40),
                y: TARGET_R + 20 + Math.random() * (PAD_H - 2 * TARGET_R - 40) };
    const clear = Math.hypot(t.x - mouse.x, t.y - mouse.y) > 200 &&
      mouse.targets.every(o => Math.hypot(t.x - o.x, t.y - o.y) > 3 * TARGET_R);
    if (clear || tries > 50) { mouse.targets.push(t); return; }
  }
}
for (let i = 0; i < 3; i++) spawnTarget();

// Clicking a target pops it; clicking anywhere else starts a stroke that follows the pointer
// until the button is released
function click() {
  const i = mouse.targets.findIndex(overTarget);
  if (i >= 0) {
    const [t] = mouse.targets.splice(i, 1);
    mouse.pops.push({ x: t.x, y: t.y, at: performance.now() });
    document.getElementById("hits").textContent = ++mouse.hits;
    spawnTarget();
  } else {
    mouse.stroke = { pts: [[mouse.x, mouse.y]], end: null };
    mouse.strokes.push(mouse.stroke);
  }
}

function updateMouse(s, dt, replay) {
  const pressed = s.b & ~mouse.btn, released = mouse.btn & ~s.b;
  mouse.btn = s.b;
  if (replay) return;  // stale history: track the buttons, but don't act on it

  if (pressed & 1) { mouse.freezeUntil = s.t + CLICK_FREEZE_MS; click(); }
  if (released & 1) {
    mouse.freezeUntil = s.t + CLICK_FREEZE_MS;
    if (mouse.stroke) { mouse.stroke.end = performance.now(); mouse.stroke = null; }
  }
  if (pressed & 2) { mouse.x = PAD_W / 2; mouse.y = PAD_H / 2; }

  // World "up" in body coordinates is the bottom row of the body→world rotation. The stick
  // points along body +Y, so "right" = forward × up = (up.z, 0, -up.x).
  const up = rotationMatrix(orient.q)[2];
  const r = [up[2], 0, -up[0]], rn = Math.hypot(...r);
  if (rn > 0.25) mouse.right = r.map(v => v / rn);  // pointing near straight up/down: keep the last one
  const yaw = dot3(orient.w, up), pitch = dot3(orient.w, mouse.right);  // °/s; turning left / nose up is positive

  if (s.t >= mouse.freezeUntil) {
    const speed = Math.hypot(yaw, pitch);
    const keep = speed * speed / (speed * speed + JITTER_DPS * JITTER_DPS);
    // Pointer acceleration: slow turns are precise (~80° to cross the pad), quick ones ~25°
    const perDeg = mouse.sens * Math.min(10 + 0.2 * speed, 40) * keep;
    mouse.x = Math.max(0, Math.min(PAD_W, mouse.x - yaw * dt * perDeg));
    mouse.y = Math.max(0, Math.min(PAD_H, mouse.y - pitch * dt * perDeg));
  }

  if (mouse.stroke) {
    const last = mouse.stroke.pts[mouse.stroke.pts.length - 1];
    if (Math.hypot(mouse.x - last[0], mouse.y - last[1]) > 2) mouse.stroke.pts.push([mouse.x, mouse.y]);
  }
}

function renderMouse() {
  document.getElementById("chip-a").classList.toggle("on", !!(mouse.btn & 1));
  document.getElementById("chip-b").classList.toggle("on", !!(mouse.btn & 2));

  const canvas = document.getElementById("pad"), dpr = window.devicePixelRatio || 1;
  const W = canvas.clientWidth, H = canvas.clientHeight;
  if (canvas.width !== W * dpr || canvas.height !== H * dpr) { canvas.width = W * dpr; canvas.height = H * dpr; }
  const ctx = canvas.getContext("2d"), k = W / PAD_W, now = performance.now();
  ctx.setTransform(dpr * k, 0, 0, dpr * k, 0, 0);
  ctx.clearRect(0, 0, PAD_W, PAD_H);
  ctx.lineCap = "round"; ctx.lineJoin = "round";

  // Strokes, fading out once the button is released
  mouse.strokes = mouse.strokes.filter(st => !st.end || now - st.end < STROKE_FADE_MS);
  ctx.strokeStyle = css("--y"); ctx.lineWidth = 6;
  for (const st of mouse.strokes) {
    ctx.globalAlpha = st.end ? 1 - (now - st.end) / STROKE_FADE_MS : 1;
    ctx.beginPath();
    st.pts.forEach(([x, y], j) => j ? ctx.lineTo(x, y) : ctx.moveTo(x, y));
    if (st.pts.length === 1) ctx.lineTo(st.pts[0][0] + 0.1, st.pts[0][1]);
    ctx.stroke();
  }

  // Targets, with a ring when the pointer is over one, and rings expanding from ones just hit
  ctx.globalAlpha = 1;
  const blue = css("--x"), surface = css("--surface");
  for (const t of mouse.targets) {
    ctx.fillStyle = blue; ctx.beginPath(); ctx.arc(t.x, t.y, TARGET_R, 0, 7); ctx.fill();
    ctx.fillStyle = surface; ctx.beginPath(); ctx.arc(t.x, t.y, TARGET_R * 0.6, 0, 7); ctx.fill();
    ctx.fillStyle = blue; ctx.beginPath(); ctx.arc(t.x, t.y, TARGET_R * 0.25, 0, 7); ctx.fill();
    if (overTarget(t)) {
      ctx.strokeStyle = blue; ctx.lineWidth = 4;
      ctx.beginPath(); ctx.arc(t.x, t.y, TARGET_R + 10, 0, 7); ctx.stroke();
    }
  }
  mouse.pops = mouse.pops.filter(p => now - p.at < POP_MS);
  for (const p of mouse.pops) {
    const f = (now - p.at) / POP_MS;
    ctx.globalAlpha = 1 - f; ctx.strokeStyle = blue; ctx.lineWidth = 5;
    ctx.beginPath(); ctx.arc(p.x, p.y, TARGET_R * (1 + 1.2 * f), 0, 7); ctx.stroke();
  }
  ctx.globalAlpha = 1;

  // Pointer: an arrow with its tip at (x, y), a constant size on screen
  ctx.save();
  ctx.translate(mouse.x, mouse.y); ctx.scale(1.3 / k, 1.3 / k);
  ctx.beginPath(); ctx.moveTo(0, 0);
  for (const [x, y] of [[0, 17], [4.5, 13], [7.5, 20], [10, 19], [7, 12], [12.5, 12]]) ctx.lineTo(x, y);
  ctx.closePath();
  ctx.strokeStyle = surface; ctx.lineWidth = 3; ctx.stroke();
  ctx.fillStyle = mouse.btn & 1 ? css("--y") : css("--ink"); ctx.fill();
  ctx.restore();
}

document.getElementById("sens").oninput = e => { mouse.sens = +e.target.value; };
document.getElementById("center").onclick = () => {
  mouse.x = PAD_W / 2; mouse.y = PAD_H / 2;
  renderMouse();
};

document.getElementById("lcd-ip").textContent = location.hostname;

function scheduleRender() {
  if (frameQueued) return;
  frameQueued = true;
  requestAnimationFrame(() => { frameQueued = false; render(); });
}

function onBatch(batch) {
  if (paused) return;
  // History for a new connection may land after the first live sample, so keep order by time
  const lastT = samples.length ? samples[samples.length - 1].t : -1;
  // A big batch is the history a new connection starts with (or a backlog after a WiFi stall).
  // It still feeds the orientation filter, but the air mouse ignores it as stale.
  const replay = batch.length > 8;
  let outOfOrder = false;
  for (const sample of batch) {
    if (samples.some(x => x.seq === sample.seq)) continue;
    if (sample.t < lastT) outOfOrder = true;
    samples.push(sample);
    const dt = updateOrientation(sample);
    if (!dt) continue;
    updatePosition(sample, Math.hypot(...orient.w), dt);
    updateMouse(sample, dt, replay);
  }
  if (outOfOrder) samples.sort((p, q) => p.t - q.t);
  const cutoff = samples[samples.length - 1].t - WINDOW_MS - 1000;
  while (samples.length && samples[0].t < cutoff) samples.shift();
  scheduleRender();
}

document.getElementById("pause").onclick = e => {
  paused = !paused;
  e.target.textContent = paused ? "Resume" : "Pause";
  setStatus(true, paused ? "Paused" : `Live · ${imu}`);
};
window.addEventListener("resize", render);
matchMedia("(prefers-color-scheme: dark)").addEventListener("change", render);
setStatus(false, "Connecting…");
connectStream({
  open() {
    samples.length = 0;  // the device may have rebooted; it resends its history
    orient.reset();
    track.reset();
    setStatus(true, paused ? "Paused" : "Live");
  },
  batch(name, batch) {
    const first = !imu;
    imu = name;
    onBatch(batch);
    if (first && !paused) setStatus(true, `Live · ${imu}`);
  },
  closed(retryMs) { setStatus(false, `Disconnected — retrying in ${(retryMs / 1000).toFixed(1)} s`); },
});
</script>
</body>
</html>
)HTML";
