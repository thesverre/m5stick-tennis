#pragma once

// Lightsaber training game served at "/saber". First-person: the stick is the saber's hilt.
// A training droid hovers in front of you, marks a spot, and fires a bolt at it; get the blade
// across the bolt's path to block it and send it back. Waves of droids, or free swing.
static const char SABER_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>M5StickC Saber</title>
<style>
:root {
  color-scheme: light;
  --surface: rgba(252,252,251,0.9); --ink: #0b0b0b; --ink-2: #52514e; --muted: #898781;
  --border: rgba(11,11,11,0.10); --accent: #2a78d6; --good: #0ca30c; --bad: #d03b3b;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    color-scheme: dark;
    --surface: rgba(26,26,25,0.86); --ink: #fff; --ink-2: #c3c2b7; --muted: #898781;
    --border: rgba(255,255,255,0.10); --accent: #3987e5;
  }
}
* { box-sizing: border-box; }
[hidden] { display: none !important; }
html, body { margin: 0; height: 100%; overflow: hidden; }
body { background: #07090d; color: var(--ink); font: 14px/1.4 system-ui, -apple-system, "Segoe UI", sans-serif; }
#view { position: fixed; inset: 0; }
#view canvas { display: block; width: 100%; height: 100%; }
#flash { position: fixed; inset: 0; pointer-events: none; opacity: 0; transition: opacity .45s;
  background: radial-gradient(ellipse at center, rgba(255,40,30,0) 35%, rgba(255,40,30,.55) 100%); }
#flash.on { opacity: 1; transition: none; }
.panel { position: fixed; background: var(--surface); border: 1px solid var(--border); border-radius: 12px;
  padding: 10px 14px; backdrop-filter: blur(6px); -webkit-backdrop-filter: blur(6px); }
#brand { top: 12px; left: 12px; display: flex; flex-direction: column; gap: 2px; }
#brand h1 { font-size: 17px; margin: 0; }
#brand a { color: var(--accent); text-decoration: none; font-size: 13px; }
.status { color: var(--ink-2); display: flex; align-items: center; gap: 6px; font-size: 13px; }
.dot { width: 8px; height: 8px; border-radius: 50%; background: var(--muted); }
.dot.ok { background: var(--good); } .dot.err { background: var(--bad); }
#hud { top: 12px; right: 12px; }
.tiles { display: grid; grid-auto-flow: column; gap: 4px 18px; }
.stat .k { color: var(--muted); font-size: 11px; text-transform: uppercase; letter-spacing: .04em; }
.stat .v { font-size: 22px; font-weight: 600; font-variant-numeric: tabular-nums; }
.stat .v small { font-size: 12px; font-weight: 400; color: var(--muted); }
#lives span { color: var(--bad); letter-spacing: 2px; }
#lives span.lost { color: var(--muted); opacity: .35; }
#droidbar { top: 12px; left: 50%; transform: translateX(-50%); width: min(260px, 40%); padding: 8px 12px; }
#droidbar .k { color: var(--ink-2); font-size: 12px; margin-bottom: 4px; }
.bar { height: 8px; border-radius: 4px; background: var(--border); overflow: hidden; }
.bar i { display: block; height: 100%; width: 100%; background: var(--bad); border-radius: 4px; transition: width .2s; }
#controls { bottom: 12px; left: 50%; transform: translateX(-50%); display: flex; align-items: center; gap: 10px;
  max-width: calc(100% - 24px); }
.hint { color: var(--ink-2); font-size: 12px; }
button { font: inherit; padding: 6px 14px; border-radius: 8px; border: 1px solid var(--border);
  background: var(--surface); color: var(--ink); cursor: pointer; white-space: nowrap; }
button.primary { background: var(--accent); border-color: var(--accent); color: #fff; }
#banner { position: fixed; top: 30%; left: 50%; transform: translate(-50%, -50%) scale(.9); pointer-events: none;
  font: 800 52px/1 system-ui, sans-serif; color: #fff; text-shadow: 0 3px 18px rgba(0,0,0,.6);
  opacity: 0; transition: opacity .25s, transform .25s; text-align: center; white-space: nowrap; }
#banner.show { opacity: 1; transform: translate(-50%, -50%) scale(1); }
#banner small { display: block; font-size: 20px; font-weight: 600; margin-top: 8px; }
.dialog { top: 50%; left: 50%; transform: translate(-50%, -50%); width: min(460px, calc(100% - 32px)); padding: 18px 20px;
  max-height: calc(100% - 32px); overflow: auto; }
.dialog h2 { margin: 0 0 8px; font-size: 20px; }
.dialog ol { margin: 0 0 12px; padding-left: 20px; color: var(--ink-2); }
.dialog li { margin-bottom: 4px; }
.label { color: var(--muted); font-size: 11px; text-transform: uppercase; letter-spacing: .04em; margin-top: 10px; }
.seg { display: flex; gap: 6px; margin: 4px 0; }
.seg button { flex: 1; }
.seg button.on { background: var(--accent); border-color: var(--accent); color: #fff; }
.desc { color: var(--ink-2); font-size: 12px; min-height: 2.9em; }
.actions { display: flex; align-items: center; gap: 10px; margin-top: 12px; }
.err { color: var(--bad); }
@media (max-width: 720px) {
  #hud { top: auto; bottom: 70px; left: 12px; right: 12px; }
  #droidbar { top: 110px; width: 60%; }
  #controls .hint { display: none; }
  #banner { font-size: 38px; }
}
</style>
</head>
<body>
<div id="view"></div>
<div id="flash"></div>

<div class="panel" id="brand">
  <h1>M5StickC Saber</h1>
  <div class="status"><span class="dot" id="dot"></span><span id="status">Connecting…</span></div>
  <a href="/">← Motion page</a>
</div>

<div class="panel" id="hud" hidden>
  <div class="tiles" id="hud-waves">
    <div class="stat"><div class="k">Lives</div><div class="v" id="lives"></div></div>
    <div class="stat"><div class="k">Score</div><div class="v" id="s-score">0</div></div>
    <div class="stat"><div class="k">Wave</div><div class="v" id="s-wave">1</div></div>
    <div class="stat"><div class="k">Best</div><div class="v" id="s-best">0</div></div>
  </div>
  <div class="tiles" id="hud-free">
    <div class="stat"><div class="k">Swing</div><div class="v"><span id="s-swing">–</span> <small>km/h</small></div></div>
    <div class="stat"><div class="k">Best swing</div><div class="v"><span id="s-bestswing">–</span> <small>km/h</small></div></div>
  </div>
</div>

<div class="panel" id="droidbar" hidden>
  <div class="k">Training droid <span id="droid-name">1</span></div>
  <div class="bar"><i id="droid-hp"></i></div>
</div>

<div class="panel" id="controls">
  <span class="hint" id="play-hint">Side button: recenter · M5 button: pause</span>
  <button id="recenter">Recenter</button>
  <button id="pause">Pause</button>
  <button id="menu-btn">Menu</button>
</div>

<div id="banner"></div>

<div class="panel dialog" id="menu">
  <h2>Ignite your saber</h2>
  <ol>
    <li>Hold the stick in your fist like a saber hilt, top end toward the blade.</li>
    <li>Point it straight at your screen and press the <b>side button</b> to line the blade up.</li>
    <li>The droid marks a red ring where it will shoot. Put the blade across that ring before the bolt gets there.</li>
  </ol>
  <div class="label">Level</div>
  <div class="seg" id="levels">
    <button data-level="easy">Easy</button><button data-level="medium">Medium</button><button data-level="hard">Hard</button>
  </div>
  <div class="desc" id="level-desc"></div>
  <div class="label">Mode</div>
  <div class="seg" id="modes"><button data-mode="waves">Waves</button><button data-mode="free">Free swing</button></div>
  <div class="desc" id="mode-desc"></div>
  <div class="label">Saber hand</div>
  <div class="seg" id="hands"><button data-hand="1">Right</button><button data-hand="-1">Left</button></div>
  <div class="err" id="menu-err"></div>
  <div class="actions"><button class="primary" id="start">Start</button><span class="hint">or press the M5 button (click Start once to get sound)</span></div>
</div>

<div class="panel dialog" id="over" hidden>
  <h2 id="over-title"></h2>
  <div class="desc" id="over-sub"></div>
  <div class="actions"><button class="primary" id="again">Play again</button><button id="to-menu">Menu</button></div>
</div>

<script src="/motion.js"></script>
<script src="/controller.js"></script>
<script>
// ---- Game model (plain JS, three.js coordinates) ----
// You stand at the origin facing the screen (−Z), eyes at EYE. The droid hovers a few metres in
// front and fires bolts through spots on the plane z = TARGET_Z, just in front of you; a bolt
// that gets past PASS_Z hits you. Lengths in m, speeds in m/s, times in ms.
const EYE = [0, 1.6, 0.1];
const GRIP = [0.12, 1.15, -0.45];   // where the hands hold the hilt with the blade pointing straight ahead (x mirrors for a left hand)
const HILT = 0.14, BLADE = 1.0;     // half the hilt's length; blade length
const TARGET_Z = -0.7, PASS_Z = -0.15;
// Head, shoulders, chest, hips (x, y) — the spots the droid aims through
const ZONES = [[0, 1.72], [-0.36, 1.5], [0.36, 1.5], [0, 1.38], [-0.3, 1.15], [0.3, 1.15]];
const DROID_R = 0.15, BOLT_R = 0.02, LIVES = 5, STEP_MS = 2;

// Levels:
//  charge — warning time between the droid marking a spot and firing
//  speed  — bolt speed; gap — pause between volleys; burst — up to this many bolts per volley
//  reach  — how far from the blade's centre line a bolt is still blocked (the real glow is ~3 cm)
//  cone   — a blocked bolt that bounces off within this many degrees of the droid hits it
//           (180 = every block does); outside it the bolt flies off where the blade sent it
//  droidSpeed — how fast it flies around; every wave is 10% faster at everything
const LEVELS = {
  easy: {
    label: "Easy", desc: "Long warnings, slow bolts, a wide blade. Every block flies straight back at the droid.",
    charge: 1200, speed: 6, gap: 1300, burst: 1, reach: 0.16, cone: 180, droidSpeed: 0.8,
  },
  medium: {
    label: "Medium", desc: "Shorter warnings, faster bolts, sometimes two at once. Hold the blade roughly square to a bolt to send it back into the droid.",
    charge: 800, speed: 9, gap: 1000, burst: 2, reach: 0.1, cone: 35, droidSpeed: 1.2,
  },
  hard: {
    label: "Hard", desc: "Quick pairs of fast bolts and a real-size blade. Only a square block sends a bolt back into the droid.",
    charge: 550, speed: 12, gap: 800, burst: 2, reach: 0.06, cone: 15, droidSpeed: 1.6,
  },
};
const MODES = {
  waves: "Beat a line of training droids, each tougher and faster than the last. Five hits and you're out.",
  free: "No droid, just you and the saber. Swing it around and listen to it hum.",
};

const settings = { level: "easy", mode: "waves" };
try { Object.assign(settings, JSON.parse(localStorage.getItem("saber") || "{}")); } catch (e) { /* defaults */ }
if (!LEVELS[settings.level]) settings.level = "easy";
if (!MODES[settings.mode]) settings.mode = "waves";
let LV = LEVELS[settings.level];
let best = {};
try { best = JSON.parse(localStorage.getItem("saber-best") || "{}"); } catch (e) { /* none yet */ }

const game = {
  simT: null, simPose: null,
  clock: 0,                // game time: runs with the simulation, stops while paused
  phase: "menu",           // "menu" | "play" | "over"
  paused: false, lit: false,
  lives: LIVES, score: 0, wave: 1, blocks: 0, nextShotAt: 0,
};
const droid = { p: [0, 1.7, -4], goal: [0, 1.7, -4], alive: false, respawnAt: null, hp: 0, maxHp: 0, charging: false, chargeStart: 0 };
const bolts = [];    // {p, v, deflected, done, target}
const targets = [];  // marked spots: {p, fireAt, fired, done}
const events = [];   // for the page: sounds, sparks, banners, numbers
const pace = () => 1 + 0.1 * (game.wave - 1);

// Stick orientation → saber pose. The hands follow the blade a little: point it left and they
// move left, raise it and they come up and in, like holding a real hilt.
function saberPose(q) {
  const M = stickMatrix(q), u = column(M, 1);
  const hand = add([GRIP[0] * ctl.hand, GRIP[1], GRIP[2]], [u[0] * 0.3, u[1] * 0.2, (u[2] + 1) * 0.15]);
  return { M, u, hand, base: add(hand, scale(u, HILT)), tip: add(hand, scale(u, HILT + BLADE)) };
}
const poseAt = t => saberPose(quatAt(t));

function recenter() {
  const ok = recenterHeading();
  if (ok) game.simPose = null;
  events.push({ type: "recenter", ok });
}

ctl.swingPoint = q => saberPose(q).tip;
ctl.onSwing = speed => events.push({ type: "swing", speed });
ctl.onPress = bit => bit === 2 ? recenter() : onM5();

// Closest points between segments p1–q1 and p2–q2 (Ericson, Real-Time Collision Detection).
// Returns the distance, the fractions along each, and the two points.
function segDist(p1, q1, p2, q2) {
  const d1 = sub(q1, p1), d2 = sub(q2, p2), r = sub(p1, p2);
  const a = dot(d1, d1), e = dot(d2, d2), f = dot(d2, r);
  let s = 0, t = 0;
  if (a < 1e-12) t = e < 1e-12 ? 0 : clamp(f / e, 0, 1);
  else {
    const c = dot(d1, r);
    if (e < 1e-12) s = clamp(-c / a, 0, 1);
    else {
      const b = dot(d1, d2), den = a * e - b * b;
      s = den > 1e-12 ? clamp((b * f - c * e) / den, 0, 1) : 0;
      t = (b * s + f) / e;
      if (t < 0) { t = 0; s = clamp(-c / a, 0, 1); }
      else if (t > 1) { t = 1; s = clamp((b - c) / a, 0, 1); }
    }
  }
  const c1 = add(p1, scale(d1, s)), c2 = add(p2, scale(d2, t));
  return { d: Math.hypot(...sub(c1, c2)), s, t, c1, c2 };
}

// ---- Droid ----
const randomGoal = () => [rand(-1.6, 1.6), rand(1.25, 2.1), rand(-4.6, -3.3)];

function appear() {
  droid.alive = true;
  droid.respawnAt = null;
  droid.maxHp = droid.hp = 2 + game.wave;
  droid.p = [rand(-1, 1), 3.2, -4.5];
  droid.goal = randomGoal();
  droid.charging = false;
  game.nextShotAt = game.clock + 1600;
  events.push({ type: "wave" });
}

// Mark 1..burst different spots, then fire at each in turn
function startVolley() {
  const n = 1 + Math.floor(Math.random() * LV.burst), zones = [...ZONES].sort(() => Math.random() - 0.5).slice(0, n);
  const charge = LV.charge / pace();
  zones.forEach(([x, y], i) => targets.push({
    p: [x + rand(-0.06, 0.06), y + rand(-0.06, 0.06), TARGET_Z], fireAt: game.clock + charge + i * 320, charge, fired: false, done: false,
  }));
  droid.charging = true;
  droid.chargeStart = game.clock;
  events.push({ type: "charge", ms: charge });
}

function stepDroid(h) {
  if (!droid.alive) {
    if (droid.respawnAt !== null && game.phase === "play" && game.clock >= droid.respawnAt) appear();
    return;
  }
  if (!droid.charging) {
    const d = sub(droid.goal, droid.p), dist = Math.hypot(...d), step = LV.droidSpeed * pace() * h / 1000;
    if (dist < 0.05) droid.goal = randomGoal();
    else droid.p = add(droid.p, scale(d, Math.min(1, step / dist)));
    if (game.phase === "play" && game.clock >= game.nextShotAt) startVolley();
    return;
  }
  // Charging or mid-volley: hold still, fire each marked spot when its time comes, and stay
  // put until every bolt is done so a blocked one can come back and hit it
  for (const tg of targets) {
    if (tg.fired || game.clock < tg.fireAt) continue;
    tg.fired = true;
    bolts.push({ p: [...droid.p], v: scale(unit(sub(tg.p, droid.p)), LV.speed * pace()), deflected: false, done: false, target: tg });
    events.push({ type: "blaster" });
  }
  if (!targets.some(tg => !tg.fired) && !bolts.some(b => !b.done)) {
    droid.charging = false;
    game.nextShotAt = game.clock + LV.gap / pace();
  }
}

function damageDroid(at) {
  droid.hp--;
  game.score += 25;
  events.push({ type: "droidHit", at });
  if (droid.hp > 0) return;
  game.score += 100 * game.wave;
  events.push({ type: "boom", at: [...droid.p] });
  droid.alive = false;
  droid.charging = false;
  for (const tg of targets) if (!tg.fired) tg.done = tg.fired = true;  // cancel shots it hadn't fired
  game.wave++;
  droid.respawnAt = game.clock + 2600;
}

function hurt() {
  game.lives--;
  events.push({ type: "hurt" });
  if (game.lives > 0) return;
  game.phase = "over";
  game.lit = false;
  const key = settings.level, isBest = game.score > (best[key] || 0);
  if (isBest) { best[key] = game.score; try { localStorage.setItem("saber-best", JSON.stringify(best)); } catch (e) { /* not saved */ } }
  events.push({ type: "over", isBest });
}

// ---- Bolts ----
// A bolt is blocked when its path over this step passes within `reach` of the blade (at the
// start or end of the step; a step is 2 ms, so even a fast blade moves only a few cm).
function blockCheck(A, B, p0, p1) {
  const r = LV.reach + BOLT_R;
  for (const P of [B, A]) {
    const d = segDist(p0, p1, P.base, P.tip);
    if (d.d < r) return { P, along: d.t, at: d.c1 };
  }
  return null;
}

// Bounce off the blade like off a cylinder: the part of the bolt's velocity across the blade
// reverses, the part along it stays, so a blade square to the bolt sends it straight back and a
// slanted one sends it off at twice the slant. The blade's own motion bends it a little. If it
// then heads within the level's cone of the droid, it flies straight into it.
function deflect(b, hit, A, B, h) {
  const a = hit.P.u, speed = Math.hypot(...b.v);
  let dir = unit(sub(scale(a, 2 * dot(b.v, a)), b.v));
  const spot = P => add(P.base, scale(sub(P.tip, P.base), hit.along));
  const vBlade = scale(sub(spot(B), spot(A)), 1000 / h);
  dir = unit(add(dir, scale(vBlade, 0.04)));
  const toDroid = unit(sub(droid.p, hit.at));
  if (droid.alive && Math.acos(clamp(dot(dir, toDroid), -1, 1)) <= LV.cone * Math.PI / 180) dir = toDroid;
  b.v = scale(dir, speed * 1.3);
  b.p = hit.at;
  b.deflected = true;
  b.target.done = true;
  game.blocks++;
  game.score += 10;
  events.push({ type: "clash", at: hit.at });
}

function stepBolts(A, B, h) {
  const s = h / 1000;
  for (const b of bolts) {
    if (b.done) continue;
    const p1 = add(b.p, scale(b.v, s));
    if (!b.deflected) {
      const hit = game.lit ? blockCheck(A, B, b.p, p1) : null;
      if (hit) { deflect(b, hit, A, B, h); continue; }
      if (p1[2] > PASS_Z) {
        b.done = b.target.done = true;
        if (game.phase === "play") hurt();
        continue;
      }
    } else if (droid.alive && segDist(b.p, p1, droid.p, droid.p).d < DROID_R + BOLT_R + 0.05) {
      b.done = true;
      damageDroid(p1);
      continue;
    } else if (p1[1] < 0 || Math.hypot(...p1) > 25) {
      b.done = true;
      if (p1[1] < 0) events.push({ type: "fizzle", at: [p1[0], 0.02, p1[2]] });
      continue;
    }
    b.p = p1;
  }
}

// Advance the game to device time `target`, sub-stepping against the interpolated saber
function simulate(target) {
  if (game.simT === null || target - game.simT > 250 || target < game.simT - 1000 || game.paused || game.phase === "menu") {
    game.simT = target;  // first frame, paused, or back from a stall / hidden tab: skip ahead
    game.simPose = null;
    return;
  }
  let A = game.simPose || poseAt(game.simT);
  while (game.simT < target) {
    const h = Math.min(STEP_MS, target - game.simT), t1 = game.simT + h, B = poseAt(t1);
    game.clock += h;
    stepDroid(h);
    stepBolts(A, B, h);
    game.simT = t1; A = B;
  }
  game.simPose = A;
  for (let i = bolts.length - 1; i >= 0; i--) if (bolts[i].done) bolts.splice(i, 1);
  for (let i = targets.length - 1; i >= 0; i--) if (targets[i].done) targets.splice(i, 1);
}

// ---- Page glue ----
function startGame() {
  LV = LEVELS[settings.level];
  Object.assign(game, { phase: "play", paused: false, lit: true, lives: LIVES, score: 0, wave: 1, blocks: 0 });
  bolts.length = targets.length = 0;
  droid.alive = false;
  droid.respawnAt = settings.mode === "waves" ? game.clock + 1200 : null;
  sfx.init();
  sfx.ignite();
  document.getElementById("menu").hidden = true;
  document.getElementById("over").hidden = true;
  document.getElementById("hud").hidden = false;
  document.getElementById("hud-waves").hidden = settings.mode !== "waves";
  document.getElementById("hud-free").hidden = settings.mode !== "free";
  document.getElementById("pause").textContent = "Pause";
  setPhaseUI();
  updateHud();
}

function setPhaseUI() {
  document.getElementById("pause").hidden = document.getElementById("play-hint").hidden = game.phase !== "play";
  document.getElementById("menu-btn").hidden = game.phase === "menu";
  document.getElementById("droidbar").hidden = !(settings.mode === "waves" && game.phase !== "menu");
}

function showMenu() {
  if (game.lit) sfx.retract();
  Object.assign(game, { phase: "menu", lit: false, paused: false });
  bolts.length = targets.length = 0;
  document.getElementById("over").hidden = true;
  document.getElementById("hud").hidden = true;
  document.getElementById("menu").hidden = false;
  setPhaseUI();
}

function togglePause() {
  if (game.phase !== "play") return;
  game.paused = !game.paused;
  document.getElementById("pause").textContent = game.paused ? "Resume" : "Pause";
  if (game.paused) banner("Paused", "#ffffff", "", 100000); else hideBanner();
}

function onM5() {
  if (game.phase === "play") togglePause();
  else startGame();
}

function renderMenu() {
  for (const b of document.querySelectorAll("#levels button")) b.classList.toggle("on", b.dataset.level === settings.level);
  for (const b of document.querySelectorAll("#modes button")) b.classList.toggle("on", b.dataset.mode === settings.mode);
  for (const b of document.querySelectorAll("#hands button")) b.classList.toggle("on", +b.dataset.hand === ctl.hand);
  document.getElementById("level-desc").textContent = LEVELS[settings.level].desc;
  document.getElementById("mode-desc").textContent = MODES[settings.mode];
  try { localStorage.setItem("saber", JSON.stringify(settings)); } catch (e) { /* not remembered */ }
}

function updateHud() {
  const lives = document.getElementById("lives");
  lives.replaceChildren(...Array.from({ length: LIVES }, (_, i) => {
    const s = document.createElement("span");
    s.textContent = "♥";
    if (i >= game.lives) s.className = "lost";
    return s;
  }));
  document.getElementById("s-score").textContent = game.score;
  document.getElementById("s-wave").textContent = game.wave;
  document.getElementById("s-best").textContent = best[settings.level] || 0;
  document.getElementById("droid-name").textContent = game.wave;
  document.getElementById("droid-hp").style.width = `${droid.maxHp ? 100 * Math.max(0, droid.hp) / droid.maxHp : 100}%`;
}

function setStatus(ok, text) {
  document.getElementById("dot").className = "dot " + (ok ? "ok" : "err");
  document.getElementById("status").textContent = text;
}

let bannerTimer = 0;
function banner(text, color, small = "", ms = 1300) {
  const el = document.getElementById("banner");
  el.textContent = text;
  if (small) { const s = document.createElement("small"); s.textContent = small; el.appendChild(s); }
  el.style.color = color;
  el.classList.add("show");
  clearTimeout(bannerTimer);
  bannerTimer = setTimeout(hideBanner, ms);
}
const hideBanner = () => document.getElementById("banner").classList.remove("show");
const kmh = v => Math.round(v * 3.6);

// ---- Sound: everything synthesized with Web Audio ----
const sfx = {
  ctx: null,
  init() {
    if (this.ctx) { this.ctx.resume(); return; }
    try { this.ctx = new AudioContext(); } catch (e) { return; }
    const ctx = this.ctx;
    this.master = ctx.createGain(); this.master.gain.value = 0.6; this.master.connect(ctx.destination);
    // Hum: two slightly detuned saws (they beat against each other) through a low-pass
    this.humGain = ctx.createGain(); this.humGain.gain.value = 0;
    this.humFilter = ctx.createBiquadFilter(); this.humFilter.type = "lowpass"; this.humFilter.frequency.value = 500;
    this.oscs = [88, 90.5].map(f => { const o = ctx.createOscillator(); o.type = "sawtooth"; o.frequency.value = f; o.connect(this.humFilter); o.start(); return o; });
    this.humFilter.connect(this.humGain).connect(this.master);
    // Swoosh: looping noise through a band-pass that opens up with swing speed
    this.noise = ctx.createBuffer(1, ctx.sampleRate, ctx.sampleRate);
    const data = this.noise.getChannelData(0);
    for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
    const src = ctx.createBufferSource(); src.buffer = this.noise; src.loop = true; src.start();
    this.swFilter = ctx.createBiquadFilter(); this.swFilter.type = "bandpass"; this.swFilter.Q.value = 1.2; this.swFilter.frequency.value = 400;
    this.swGain = ctx.createGain(); this.swGain.gain.value = 0;
    src.connect(this.swFilter).connect(this.swGain).connect(this.master);
  },
  ready() { return this.ctx && this.ctx.state === "running"; },
  ignite() { this.sweep(160, 900, 0.35, 0.18, "sawtooth"); },
  retract() { this.sweep(700, 90, 0.35, 0.15, "sawtooth"); },
  // Called every frame with the blade tip's speed: hums while lit, louder and higher when swung
  swing(speed) {
    if (!this.ready()) return;
    const t = this.ctx.currentTime, on = game.lit && !game.paused, k = on ? Math.min(1, speed / 12) : 0;
    this.humGain.gain.setTargetAtTime(on ? 0.12 + 0.12 * k : 0, t, on ? 0.03 : 0.08);
    this.oscs[0].frequency.setTargetAtTime(88 * (1 + 0.5 * k), t, 0.03);
    this.oscs[1].frequency.setTargetAtTime(90.5 * (1 + 0.5 * k), t, 0.03);
    this.humFilter.frequency.setTargetAtTime(500 + 1500 * k, t, 0.03);
    this.swGain.gain.setTargetAtTime(0.5 * k * k, t, 0.03);
    this.swFilter.frequency.setTargetAtTime(300 + 1400 * k, t, 0.03);
  },
  sweep(f0, f1, len, gain, type = "square") {
    if (!this.ready()) return;
    const ctx = this.ctx, t = ctx.currentTime, o = ctx.createOscillator(), g = ctx.createGain();
    o.type = type;
    o.frequency.setValueAtTime(f0, t);
    o.frequency.exponentialRampToValueAtTime(f1, t + len);
    g.gain.setValueAtTime(gain, t);
    g.gain.exponentialRampToValueAtTime(0.001, t + len);
    o.connect(g).connect(this.master);
    o.start(t); o.stop(t + len + 0.02);
  },
  burst(freq, q, len, gain, type = "bandpass") {
    if (!this.ready()) return;
    const ctx = this.ctx, t = ctx.currentTime, src = ctx.createBufferSource(), f = ctx.createBiquadFilter(), g = ctx.createGain();
    src.buffer = this.noise;
    f.type = type; f.frequency.value = freq; f.Q.value = q;
    g.gain.setValueAtTime(gain, t);
    g.gain.exponentialRampToValueAtTime(0.001, t + len);
    src.connect(f).connect(g).connect(this.master);
    src.start(t, Math.random() * 0.5); src.stop(t + len + 0.02);
  },
  clash() { this.burst(2600, 3, 0.18, 0.7); this.sweep(1500, 700, 0.12, 0.12, "square"); },
  blaster() { this.sweep(1600, 260, 0.18, 0.14, "square"); },
  charge(ms) { this.sweep(260, 900, ms / 1000, 0.04, "sine"); },
  hurt() { this.sweep(130, 45, 0.35, 0.6, "sine"); this.burst(300, 0.8, 0.25, 0.5, "lowpass"); },
  droidHit() { this.burst(4200, 1, 0.14, 0.5, "highpass"); this.sweep(900, 400, 0.1, 0.12, "square"); },
  boom() { this.burst(260, 0.7, 0.9, 1, "lowpass"); this.sweep(200, 40, 0.8, 0.5, "sine"); },
};

// Hand game events to the page (sounds, banners, numbers); sparks are handled by the view
function drainEvents(onEffect) {
  for (const e of events.splice(0)) {
    if (e.type === "clash") { sfx.clash(); onEffect(e); updateHud(); }
    else if (e.type === "blaster") sfx.blaster();
    else if (e.type === "charge") sfx.charge(e.ms);
    else if (e.type === "hurt") {
      sfx.hurt(); onEffect(e); updateHud();
      const f = document.getElementById("flash");
      f.classList.add("on"); requestAnimationFrame(() => requestAnimationFrame(() => f.classList.remove("on")));
    } else if (e.type === "droidHit") { sfx.droidHit(); onEffect(e); updateHud(); }
    else if (e.type === "boom") { sfx.boom(); onEffect(e); updateHud(); banner("Droid down!", "#7dff8a", `+${100 * (game.wave - 1)}`, 1800); }
    else if (e.type === "fizzle") onEffect(e);
    else if (e.type === "wave") { updateHud(); banner(`Wave ${game.wave}`, "#ffffff", `Droid with ${droid.maxHp} hits`); }
    else if (e.type === "swing") {
      document.getElementById("s-swing").textContent = kmh(e.speed);
      document.getElementById("s-bestswing").textContent = kmh(ctl.bestSwing);
    } else if (e.type === "over") {
      sfx.retract();
      setPhaseUI();
      document.getElementById("over-title").textContent = `Defeated on wave ${game.wave}`;
      document.getElementById("over-sub").textContent =
        `${LV.label}: ${game.score} points, ${game.blocks} bolts blocked.${e.isBest ? " New best!" : ` Best: ${best[settings.level] || 0}.`} Press the M5 button or Play again for another go.`;
      setTimeout(() => { if (game.phase === "over") document.getElementById("over").hidden = false; }, 1200);
    } else if (e.type === "recenter") {
      banner(e.ok ? "Centered" : "Point at the screen", "#ffffff", e.ok ? "" : "then press the side button again");
    }
  }
}

for (const b of document.querySelectorAll("#levels button")) b.onclick = () => { settings.level = b.dataset.level; renderMenu(); };
for (const b of document.querySelectorAll("#modes button")) b.onclick = () => { settings.mode = b.dataset.mode; renderMenu(); };
for (const b of document.querySelectorAll("#hands button")) b.onclick = () => { setHand(+b.dataset.hand); renderMenu(); };
document.getElementById("start").onclick = startGame;
document.getElementById("again").onclick = startGame;
document.getElementById("to-menu").onclick = showMenu;
document.getElementById("menu-btn").onclick = showMenu;
document.getElementById("recenter").onclick = recenter;
document.getElementById("pause").onclick = togglePause;
renderMenu();
setPhaseUI();

let imu = "";
setStatus(false, "Connecting…");
connectStream({
  open() {
    resetStream();
    game.simT = null;
    setStatus(true, "Live");
  },
  batch(name, batch) {
    if (!imu) setStatus(true, `Live · ${name}`);
    imu = name;
    feedBatch(batch);
  },
  closed(retryMs) { setStatus(false, `Disconnected — retrying in ${(retryMs / 1000).toFixed(1)} s`); },
});

// ---- 3D view ----
import("https://cdn.jsdelivr.net/npm/three@0.170.0/build/three.module.min.js").then(buildScene, () => {
  document.getElementById("menu-err").textContent =
    "Couldn't load the 3D library. This page needs internet access for three.js.";
});

function buildScene(THREE) {
  const V = a => new THREE.Vector3(a[0], a[1], a[2]);
  const BLADE_COLOR = 0x3aa0ff, BOLT_COLOR = 0xff3322;
  const view = document.getElementById("view");
  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  view.appendChild(renderer.domElement);

  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0x07090d);
  scene.fog = new THREE.Fog(0x07090d, 7, 24);
  const camera = new THREE.PerspectiveCamera(70, 1, 0.03, 100);
  scene.add(new THREE.HemisphereLight(0x6c7f99, 0x0b0d12, 0.9));
  const key = new THREE.DirectionalLight(0xbfd4ff, 0.8);
  key.position.set(-2, 5, 3);
  scene.add(key);

  // Training room: dark floor with a faint grid, a lit platform under you, and a ring of
  // light pillars in the distance for depth
  const floor = new THREE.Mesh(new THREE.CircleGeometry(30, 64), new THREE.MeshStandardMaterial({ color: 0x0e1218, roughness: 0.85 }));
  floor.rotation.x = -Math.PI / 2;
  scene.add(floor);
  const grid = new THREE.GridHelper(40, 40, 0x1c3552, 0x121f30);
  grid.position.y = 0.002;
  scene.add(grid);
  const pad = new THREE.Mesh(new THREE.RingGeometry(0.9, 0.95, 64), new THREE.MeshBasicMaterial({ color: 0x2a6fd6, transparent: true, opacity: 0.6 }));
  pad.rotation.x = -Math.PI / 2; pad.position.y = 0.004;
  scene.add(pad);
  const pillarMat = new THREE.MeshBasicMaterial({ color: 0x1d4a7a });
  for (let i = 0; i < 18; i++) {
    const a = (i / 18) * Math.PI * 2, pillar = new THREE.Mesh(new THREE.BoxGeometry(0.12, 4, 0.12), pillarMat);
    pillar.position.set(Math.sin(a) * 11, 2, -Math.cos(a) * 11);
    scene.add(pillar);
  }

  // Additive glow shells around a bright core make the blade and bolts look lit
  const glowMat = (color, opacity) => new THREE.MeshBasicMaterial({ color, transparent: true, opacity, blending: THREE.AdditiveBlending, depthWrite: false });
  const layers = (group, len, radii) => radii.forEach(([r, mat]) => {
    const m = new THREE.Mesh(new THREE.CapsuleGeometry(r, len, 4, 16), mat);
    m.position.y = len / 2;
    group.add(m);
  });

  // Saber, built in stick coordinates: Y along the hilt and blade, hands at 0
  const saber = new THREE.Group();
  saber.matrixAutoUpdate = false;
  const metal = new THREE.MeshStandardMaterial({ color: 0xa9b2bc, metalness: 0.85, roughness: 0.3 });
  const dark = new THREE.MeshStandardMaterial({ color: 0x16181c, roughness: 0.6 });
  const hiltBody = new THREE.Mesh(new THREE.CylinderGeometry(0.018, 0.02, 2 * HILT, 16), metal);
  saber.add(hiltBody);
  for (let i = 0; i < 6; i++) {
    const ring = new THREE.Mesh(new THREE.CylinderGeometry(0.021, 0.021, 0.012, 16), dark);
    ring.position.y = -0.08 + i * 0.026;
    saber.add(ring);
  }
  const emitter = new THREE.Mesh(new THREE.CylinderGeometry(0.024, 0.019, 0.04, 16), metal);
  emitter.position.y = HILT - 0.02;
  const button = new THREE.Mesh(new THREE.BoxGeometry(0.01, 0.018, 0.012), new THREE.MeshBasicMaterial({ color: 0xd03b3b }));
  button.position.set(0, 0.06, 0.02);
  saber.add(emitter, button);
  const blade = new THREE.Group();
  blade.position.y = HILT;
  layers(blade, BLADE, [[0.011, new THREE.MeshBasicMaterial({ color: 0xf2f8ff })], [0.022, glowMat(BLADE_COLOR, 0.55)],
                        [0.042, glowMat(BLADE_COLOR, 0.22)], [0.08, glowMat(BLADE_COLOR, 0.07)]]);
  const bladeLight = new THREE.PointLight(BLADE_COLOR, 2.2, 4.5, 1.6);
  blade.add(bladeLight);
  saber.add(blade);
  scene.add(saber);
  let bladeLen = 0;  // 0 = retracted, 1 = fully lit

  // Swoosh behind the blade while it moves fast
  const TRAIL = 16, trailPos = new Float32Array(TRAIL * 2 * 3), trailCol = new Float32Array(TRAIL * 2 * 4);
  const trailGeo = new THREE.BufferGeometry();
  trailGeo.setAttribute("position", new THREE.BufferAttribute(trailPos, 3));
  trailGeo.setAttribute("color", new THREE.BufferAttribute(trailCol, 4));
  const idx = [];
  for (let i = 0; i < TRAIL - 1; i++) { const a = 2 * i; idx.push(a, a + 1, a + 2, a + 1, a + 3, a + 2); }
  trailGeo.setIndex(idx);
  const trail = new THREE.Mesh(trailGeo, new THREE.MeshBasicMaterial({ vertexColors: true, transparent: true, side: THREE.DoubleSide,
    depthWrite: false, blending: THREE.AdditiveBlending }));
  trail.frustumCulled = false;
  scene.add(trail);
  const trailHist = [];

  // The droid: a dark metal ball with a band, blaster ports, an antenna and a red eye that
  // brightens while it charges
  const bot = new THREE.Group();
  const shell = new THREE.MeshStandardMaterial({ color: 0x3a4049, metalness: 0.8, roughness: 0.35 });
  bot.add(new THREE.Mesh(new THREE.SphereGeometry(DROID_R, 32, 20), shell));
  const band = new THREE.Mesh(new THREE.TorusGeometry(DROID_R * 1.02, 0.012, 8, 40), dark);
  band.rotation.x = Math.PI / 2;
  bot.add(band);
  for (let i = 0; i < 6; i++) {
    const a = (i / 6) * Math.PI * 2, port = new THREE.Mesh(new THREE.CylinderGeometry(0.018, 0.022, 0.05, 10), dark);
    port.position.set(Math.cos(a) * DROID_R, 0, Math.sin(a) * DROID_R);
    port.lookAt(0, 0, 0); port.rotateX(Math.PI / 2);
    bot.add(port);
  }
  const antenna = new THREE.Mesh(new THREE.CylinderGeometry(0.004, 0.004, 0.16, 6), metal);
  antenna.position.set(0.05, DROID_R + 0.07, 0);
  bot.add(antenna);
  const eyeMat = new THREE.MeshBasicMaterial({ color: 0xff2a1a });
  const eye = new THREE.Mesh(new THREE.SphereGeometry(0.035, 16, 12), eyeMat);
  eye.position.z = DROID_R * 0.92;
  const eyeGlow = new THREE.Mesh(new THREE.SphereGeometry(0.09, 16, 12), glowMat(0xff2a1a, 0.2));
  eyeGlow.position.z = DROID_R;
  const droidLight = new THREE.PointLight(0xff3322, 0, 3, 1.5);
  droidLight.position.z = 0.3;
  bot.add(eye, eyeGlow, droidLight);
  scene.add(bot);

  // Pools for bolts and the rings marking where they'll come through
  const boltPool = [], ringPool = [];
  const boltMesh = () => {
    const g = new THREE.Group(), inner = new THREE.Group();
    inner.position.y = -0.2;
    layers(inner, 0.4, [[0.009, new THREE.MeshBasicMaterial({ color: 0xffe4e0 })], [0.022, glowMat(BOLT_COLOR, 0.6)], [0.05, glowMat(BOLT_COLOR, 0.15)]]);
    g.add(inner);
    scene.add(g);
    return g;
  };
  const ringMesh = () => {
    const m = new THREE.Mesh(new THREE.RingGeometry(0.075, 0.09, 40), glowMat(0xff3322, 0.8));
    scene.add(m);
    return m;
  };
  const pooled = (pool, make, n) => { while (pool.length < n) pool.push(make()); pool.forEach((m, i) => { m.visible = i < n; }); return pool; };

  // Sparks: additive points that fade to black (= invisible) as they die
  const MAX_SPARKS = 400, sparkPos = new Float32Array(MAX_SPARKS * 3), sparkCol = new Float32Array(MAX_SPARKS * 3);
  const sparkGeo = new THREE.BufferGeometry();
  sparkGeo.setAttribute("position", new THREE.BufferAttribute(sparkPos, 3));
  sparkGeo.setAttribute("color", new THREE.BufferAttribute(sparkCol, 3));
  const sparkPts = new THREE.Points(sparkGeo, new THREE.PointsMaterial({ size: 0.025, vertexColors: true, transparent: true,
    blending: THREE.AdditiveBlending, depthWrite: false }));
  sparkPts.frustumCulled = false;
  scene.add(sparkPts);
  const sparks = [];
  function spray(at, n, speed, color, life) {
    const c = new THREE.Color(color);
    for (let i = 0; i < n && sparks.length < MAX_SPARKS; i++) {
      const d = unit([rand(-1, 1), rand(-1, 1), rand(-1, 1)]);
      sparks.push({ p: [...at], v: scale(d, speed * rand(0.3, 1)), c, life, age: 0 });
    }
  }
  let shakeUntil = 0;
  function onEffect(e) {
    if (e.type === "clash") spray(e.at, 40, 3, 0xbfe2ff, 0.35);
    else if (e.type === "droidHit") spray(e.at, 50, 2.5, 0xffb070, 0.5);
    else if (e.type === "boom") { spray(e.at, 200, 4, 0xff8a40, 0.9); spray(e.at, 60, 2, 0xffffff, 0.5); }
    else if (e.type === "fizzle") spray(e.at, 12, 1.2, 0xff5a40, 0.3);
    else if (e.type === "hurt") shakeUntil = performance.now() + 250;
  }

  function resize() {
    const w = window.innerWidth, h = window.innerHeight;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.fov = w / h < 1 ? 88 : 70;  // portrait screens: widen so the whole guard zone fits
    camera.updateProjectionMatrix();
  }
  window.addEventListener("resize", resize);
  resize();
  document.addEventListener("pointerdown", () => sfx.init(), { once: true });

  let lastFrame = performance.now();
  function frame() {
    requestAnimationFrame(frame);
    const now = performance.now(), dt = Math.min(0.1, (now - lastFrame) / 1000);
    lastFrame = now;
    drainEvents(onEffect);
    let tipSpeed = 0;
    if (streamReady()) {
      simulate(deviceNow());
      drainEvents(onEffect);
      const P = game.simPose || poseAt(deviceNow()), M = P.M, hd = P.hand;
      saber.matrix.set(M[0][0], M[0][1], M[0][2], hd[0], M[1][0], M[1][1], M[1][2], hd[1], M[2][0], M[2][1], M[2][2], hd[2], 0, 0, 0, 1);
      saber.matrixWorldNeedsUpdate = true;
      trailHist.unshift({ base: P.base, tip: add(P.base, scale(sub(P.tip, P.base), bladeLen)) });
      trailHist.length = Math.min(trailHist.length, TRAIL);
      tipSpeed = trailHist.length > 1 ? Math.hypot(...sub(trailHist[0].tip, trailHist[1].tip)) / Math.max(dt, 0.001) : 0;
    }

    // Blade: grows out of the emitter when lit, slides back when not
    bladeLen = clamp(bladeLen + (game.lit ? dt / 0.25 : -dt / 0.3), 0, 1);
    blade.visible = bladeLen > 0.01;
    blade.scale.set(1, Math.max(0.001, bladeLen), 1);
    bladeLight.position.y = BLADE / 2;
    sfx.swing(tipSpeed);
    const c = new THREE.Color(BLADE_COLOR);
    for (let i = 0; i < TRAIL; i++) {
      const e = trailHist[Math.min(i, trailHist.length - 1)];
      const alpha = e ? (1 - i / TRAIL) * clamp((tipSpeed - 2) / 8, 0, 1) * 0.5 * bladeLen : 0;
      if (e) { trailPos.set(e.base, i * 6); trailPos.set(e.tip, i * 6 + 3); }
      trailCol.set([c.r * alpha, c.g * alpha, c.b * alpha, alpha, c.r * alpha, c.g * alpha, c.b * alpha, alpha], i * 8);
    }
    trailGeo.attributes.position.needsUpdate = trailGeo.attributes.color.needsUpdate = true;

    // Droid: bobs gently, faces you, eye flares while it charges
    const showDroid = settings.mode === "waves" && (droid.alive || game.phase === "menu");
    bot.visible = showDroid;
    if (showDroid) {
      bot.position.set(droid.p[0], droid.p[1] + Math.sin(now / 400) * 0.03, droid.p[2]);
      bot.lookAt(...EYE);
      const ch = droid.charging ? clamp((game.clock - droid.chargeStart) / (LV.charge / pace()), 0, 1) : 0;
      eyeGlow.scale.setScalar(1 + ch * 1.5);
      eyeGlow.material.opacity = 0.2 + ch * 0.5;
      droidLight.intensity = ch * 3;
    }

    const bp = pooled(boltPool, boltMesh, bolts.length);
    bolts.forEach((b, i) => {
      bp[i].position.copy(V(b.p));
      bp[i].quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), V(unit(b.v)));
    });
    // Marked spots: the ring shrinks onto the spot as the shot comes, and pulses once fired
    const live = targets.filter(t => !t.done);
    const rp = pooled(ringPool, ringMesh, live.length);
    live.forEach((t, i) => {
      rp[i].position.copy(V(t.p));
      rp[i].lookAt(...EYE);
      const left = t.fired ? 0 : clamp((t.fireAt - game.clock) / t.charge, 0, 1);
      rp[i].scale.setScalar(t.fired ? 1 + 0.15 * Math.sin(now / 40) : 1 + 2.2 * left);
      rp[i].material.opacity = t.fired ? 0.95 : 0.35 + 0.5 * (1 - left);
    });

    for (let i = sparks.length - 1; i >= 0; i--) {
      const s = sparks[i];
      s.age += dt;
      if (s.age >= s.life) { sparks.splice(i, 1); continue; }
      s.v[1] -= 4 * dt;
      s.p = add(s.p, scale(s.v, dt));
    }
    for (let i = 0; i < MAX_SPARKS; i++) {
      const s = sparks[i], k = s ? 1 - s.age / s.life : 0;
      if (s) sparkPos.set(s.p, i * 3);
      sparkCol.set(s ? [s.c.r * k, s.c.g * k, s.c.b * k] : [0, 0, 0], i * 3);
    }
    sparkGeo.attributes.position.needsUpdate = sparkGeo.attributes.color.needsUpdate = true;

    const shake = now < shakeUntil ? 0.02 * (shakeUntil - now) / 250 : 0;
    camera.position.set(EYE[0] + rand(-shake, shake), EYE[1] + rand(-shake, shake), EYE[2]);
    camera.lookAt(0, 1.25, -3);
    renderer.render(scene, camera);
  }
  requestAnimationFrame(frame);
}
</script>
</body>
</html>
)HTML";
