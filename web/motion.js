// Shared script at "/motion.js": the IMU sample stream from /ws and the orientation
// filter. Loaded by every page, so they all use the same fusion.

// ---- Stream ----
// Opens the /ws WebSocket and reconnects with backoff. The device sends
// {"imu":"MPU6886","s":[[seq,t,gx,gy,gz,ax,ay,az,btn],...]}; each sample is handed on as
// {seq, t (ms), g: [x,y,z] (°/s), a: [x,y,z] (g), b (button bits: 1 = M5, 2 = side)}.
// handlers: open(), batch(imu, samples), closed(retryMs)
function connectStream(handlers) {
  let retryMs = 500;
  (function connect() {
    const ws = new WebSocket(`ws://${location.host}/ws`);
    ws.onopen = () => { retryMs = 500; handlers.open(); };
    ws.onmessage = e => {
      const d = JSON.parse(e.data);
      handlers.batch(d.imu, d.s.map(s => ({ seq: s[0], t: s[1], g: [s[2], s[3], s[4]], a: [s[5], s[6], s[7]], b: s[8] || 0 })));
    };
    ws.onclose = () => {
      handlers.closed(retryMs);
      setTimeout(connect, retryMs);
      retryMs = Math.min(retryMs * 2, 8000);
    };
  })();
}

// ---- Orientation: Mahony filter fusing gyro + accelerometer ----
// q rotates stick (body) coordinates into world coordinates, world Z = up.
// Body axes as reported by M5Unified: X across the stick, Y along it toward
// the screen end, Z out of the screen.
const DEG = Math.PI / 180;
const KP = 1.0, KI = 0.02;
const orient = {
  q: null, integ: [0, 0, 0], bias: [0, 0, 0], lastT: null,
  aAvg: [0, 0, 1], aDev: 0,         // smoothed raw acceleration vector (body frame), and how far a is from it
  gAvg: [0, 0, 0],                  // smoothed raw gyro rate
  biasStillMs: 0, biasLearnedMs: 0, // stillness streak for bias learning, and total time spent learning
  w: [0, 0, 0],                     // latest bias-corrected rotation rate, °/s (body frame)
  reset() {
    this.q = null; this.integ = [0, 0, 0]; this.lastT = null; this.aAvg = [0, 0, 1];
    this.biasStillMs = 0; this.w = [0, 0, 0];
  },
};

function quatFromAccel(a) {
  // Tilt from gravity, heading zero (stick pointing "away" on the page)
  const roll = Math.atan2(a[1], a[2]);
  const pitch = Math.atan2(-a[0], Math.hypot(a[1], a[2]));
  const cr = Math.cos(roll / 2), sr = Math.sin(roll / 2), cp = Math.cos(pitch / 2), sp = Math.sin(pitch / 2);
  return [cr * cp, sr * cp, cr * sp, -sr * sp];
}

function quatMul([a0, a1, a2, a3], [b0, b1, b2, b3]) {
  return [
    a0 * b0 - a1 * b1 - a2 * b2 - a3 * b3,
    a0 * b1 + a1 * b0 + a2 * b3 - a3 * b2,
    a0 * b2 - a1 * b3 + a2 * b0 + a3 * b1,
    a0 * b3 + a1 * b2 - a2 * b1 + a3 * b0,
  ];
}

// Returns the time step used (s), or 0 when the sample only initialized or was skipped.
function updateOrientation(s) {
  if (!orient.q) {
    orient.q = quatFromAccel(s.a); orient.lastT = s.t;
    orient.aAvg = [...s.a];
    orient.gAvg = [...s.g];
    return 0;
  }
  if (s.t <= orient.lastT) return 0;
  const dt = Math.min((s.t - orient.lastT) / 1000, 0.1);
  orient.lastT = s.t;

  const aMag = Math.hypot(...s.a);
  const ka = Math.min(1, dt / 0.5), kg = Math.min(1, dt / 0.2);
  for (let i = 0; i < 3; i++) {
    orient.aAvg[i] += (s.a[i] - orient.aAvg[i]) * ka;
    orient.gAvg[i] += (s.g[i] - orient.gAvg[i]) * kg;
  }
  orient.aDev = Math.hypot(s.a[0] - orient.aAvg[0], s.a[1] - orient.aAvg[1], s.a[2] - orient.aAvg[2]);

  // Learn gyro bias while the stick is still, so heading and the air-mouse pointer drift less.
  // Stillness is judged on the smoothed, bias-corrected rate (this unit's raw offset alone is
  // ~4 °/s, and single samples are noisy) plus a steady gravity vector. Until a bias has been
  // learned, a larger residual is allowed so learning can start at all. The bias is pulled
  // toward the smoothed rate too: the smoothing lags, so the first samples of a sudden turn
  // still pass as quiet, and their raw rate would leak into the bias.
  const gRes = Math.hypot(...orient.gAvg.map((v, i) => v - orient.bias[i]));
  const quiet = gRes < (orient.biasLearnedMs > 1000 ? 2.5 : 8) && orient.aDev < 0.03 && Math.abs(aMag - 1) < 0.08;
  orient.biasStillMs = quiet ? orient.biasStillMs + dt * 1000 : 0;
  if (orient.biasStillMs > 400) {
    const kb = Math.min(1, dt / (orient.biasLearnedMs > 1000 ? 2 : 0.3));
    for (let i = 0; i < 3; i++) orient.bias[i] += (orient.gAvg[i] - orient.bias[i]) * kb;
    orient.biasLearnedMs += dt * 1000;
  }
  orient.w = s.g.map((v, i) => v - orient.bias[i]);
  let [gx, gy, gz] = orient.w.map(v => v * DEG);
  const [q0, q1, q2, q3] = orient.q;

  // Correct toward the measured gravity direction. While the stick is being pushed or swung
  // the accelerometer no longer points at gravity, so its weight fades out as |a| leaves 1 g.
  const trust = Math.max(0, 1 - Math.abs(aMag - 1) / 0.15);
  if (trust > 0) {
    const [ax, ay, az] = s.a.map(v => v / aMag);
    const vx = 2 * (q1 * q3 - q0 * q2), vy = 2 * (q0 * q1 + q2 * q3), vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;
    const e = [ay * vz - az * vy, az * vx - ax * vz, ax * vy - ay * vx];
    for (let i = 0; i < 3; i++) orient.integ[i] += trust * KI * e[i] * dt;
    gx += trust * KP * e[0] + orient.integ[0];
    gy += trust * KP * e[1] + orient.integ[1];
    gz += trust * KP * e[2] + orient.integ[2];
  }

  // Rotate by the whole step at once, q ⊗ exp(ω dt / 2). Exact for a constant rate, so it
  // stays accurate during swings, where one 10 ms step can turn the stick 10° or more.
  const wn = Math.hypot(gx, gy, gz), half = 0.5 * wn * dt;
  if (wn > 1e-9) {
    const k = Math.sin(half) / wn;
    const q = quatMul(orient.q, [Math.cos(half), gx * k, gy * k, gz * k]);
    const n = Math.hypot(...q);
    orient.q = q.map(v => v / n);
  }
  return dt;
}

function rotationMatrix([w, x, y, z]) {
  return [
    [1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)],
    [2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)],
    [2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)],
  ];
}
