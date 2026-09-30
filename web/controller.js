// Shared script at "/controller.js", used by the 3D games on top of /motion.js: the
// stick as a hand-held controller. Keeps a short history of its orientation so a game can
// sample the pose at any moment, keeps game time in step with the stick's clock, and reports
// button presses and swings.

const add = (a, b) => [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
const scale = (a, k) => [a[0] * k, a[1] * k, a[2] * k];
const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
const unit = a => scale(a, 1 / Math.hypot(...a));
const mix = (a, b, f) => [a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f, a[2] + (b[2] - a[2]) * f];
const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
const rand = (lo, hi) => lo + Math.random() * (hi - lo);

const LAG_MS = 30;  // play back this far behind the newest sample, so there's data on both sides

// Which hand holds the stick: 1 = right, −1 = left. Remembered in the browser and shared by
// the games, since they're served from the same address.
function setHand(h) {
  ctl.hand = h;
  try { localStorage.setItem("hand", h < 0 ? "left" : "right"); } catch (e) { /* not remembered */ }
}

const ctl = {
  yaw0: 0,             // heading that counts as "straight ahead" (world frame, radians)
  lastSeq: -1, btn: 0,
  poses: [],           // recent {t (device ms), q} from the orientation filter
  offset: null,        // local ms − device ms, from the fastest-arriving samples
  swingPoint: null,    // q => the point whose speed counts as swing speed (a racket head, a blade tip)
  swing: { active: false, peak: 0, prev: null }, bestSwing: 0,
  onPress: () => {},   // (1 = M5 button, 2 = side button)
  onSwing: () => {},   // (peak speed in m/s) when a swing ends
  onSample: () => {},  // (sample, dt) for each live sample, after the filter has taken it in
  hand: 1,
};
try { ctl.hand = localStorage.getItem("hand") === "left" ? -1 : 1; } catch (e) { /* right */ }

// Stick body axes → three.js space (X right, Y up, Z toward the viewer), turned so the
// recentered heading points into the screen (−Z). Columns: body X, body Y (along the stick,
// toward its top), body Z (out of its screen).
function stickMatrix(q) {
  const R = rotationMatrix(q);
  const c = Math.cos(-ctl.yaw0), s = Math.sin(-ctl.yaw0);
  const M = [[0, 0, 0], [0, 0, 0], [0, 0, 0]];
  for (let col = 0; col < 3; col++) {
    const wx = R[0][col], wy = R[1][col], wz = R[2][col];
    const hx = c * wx - s * wy, hy = s * wx + c * wy;  // turn so the recentered heading is +Y
    M[0][col] = hx; M[1][col] = wz; M[2][col] = -hy;    // world (Z up, Y ahead) → three (Y up, −Z ahead)
  }
  return M;
}
const column = (M, i) => [M[0][i], M[1][i], M[2][i]];

function nlerp(a, b, f) {
  const sgn = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3] < 0 ? -1 : 1;
  const q = a.map((v, i) => v + (sgn * b[i] - v) * f), n = Math.hypot(...q);
  return q.map(v => v / n);
}

// Orientation at device time t, interpolated between samples
function quatAt(t) {
  const p = ctl.poses, n = p.length;
  if (t >= p[n - 1].t) return p[n - 1].q;
  let j = n - 1;
  while (j > 0 && p[j - 1].t > t) j--;
  if (j === 0) return p[0].q;
  return nlerp(p[j - 1].q, p[j].q, (t - p[j - 1].t) / (p[j].t - p[j - 1].t));
}

// Make the stick's current pointing direction "straight ahead". Returns false when it points
// too steeply up or down for a heading to mean anything.
function recenterHeading() {
  if (!orient.q) return false;
  const R = rotationMatrix(orient.q);
  if (Math.hypot(R[0][1], R[1][1]) < 0.5) return false;
  ctl.yaw0 = Math.atan2(-R[0][1], R[1][1]);
  ctl.swing.prev = null;  // the pose jumps; don't let that count as a swing
  return true;
}

function feedBatch(batch) {
  // A big batch is history (on connect) or a backlog after a stall: feed the filter, but
  // don't act on its buttons or swings
  const replay = batch.length > 8;
  const newest = batch[batch.length - 1];
  const arrive = performance.now() - newest.t;
  ctl.offset = ctl.offset === null || arrive < ctl.offset ? arrive : ctl.offset + (arrive - ctl.offset) * 0.002;

  for (const s of batch) {
    if (s.seq <= ctl.lastSeq) continue;  // history can overlap the live stream
    ctl.lastSeq = s.seq;
    const dt = updateOrientation(s);
    ctl.poses.push({ t: s.t, q: [...orient.q] });

    const pressed = s.b & ~ctl.btn;
    ctl.btn = s.b;
    if (replay || !dt) continue;
    if (pressed & 2) ctl.onPress(2);
    if (pressed & 1) ctl.onPress(1);
    ctl.onSample(s, dt);

    if (!ctl.swingPoint) continue;
    const p = ctl.swingPoint(orient.q), sw = ctl.swing;
    if (sw.prev) {
      const speed = Math.hypot(...sub(p, sw.prev)) / dt;
      if (speed > 4) { sw.active = true; sw.peak = Math.max(sw.peak, speed); }
      else if (sw.active && speed < 2) {
        sw.active = false;
        ctl.bestSwing = Math.max(ctl.bestSwing, sw.peak);
        ctl.onSwing(sw.peak);
        sw.peak = 0;
      }
    }
    sw.prev = p;
  }
  while (ctl.poses.length > 2 && ctl.poses[0].t < newest.t - 2000) ctl.poses.shift();
}

// On (re)connect: the device may have rebooted, so its clock and sequence numbers restart
function resetStream() {
  orient.reset();
  ctl.lastSeq = -1; ctl.poses.length = 0; ctl.offset = null; ctl.swing.prev = null;
}

const deviceNow = () => performance.now() - LAG_MS - ctl.offset;
const streamReady = () => ctl.offset !== null && ctl.poses.length > 0;
