class_name Stick
extends RefCounted
## The M5StickC as a motion source: reads its sample stream from ws://<host>/ws, fuses gyro and
## accelerometer into an orientation, and keeps a short history of it so a game can sample the
## pose at any moment in the stick's own time. A port of web/motion.js and web/controller.js.
##
## The stick sends {"imu":"MPU6886","s":[[seq,t,gx,gy,gz,ax,ay,az,btn],...]}; each sample is
## handed on as {seq, t (ms), g (°/s), a (g), b (button bits: 1 = M5, 2 = side)}.

signal sample(s: Dictionary, dt: float)  ## each live sample, after the filter has taken it in
signal pressed(bit: int)                 ## 1 = M5 button, 2 = side button

const LAG_MS := 30.0  # play back this far behind the newest sample, so there's data on both sides
const KP := 1.0
const KI := 0.02

var host := "m5stick.local"
var imu := ""
var status := "Connecting…"
var connected := false

# Orientation: q rotates stick (body) coordinates into world coordinates, world Z = up
var q := Quaternion.IDENTITY
var w := Vector3.ZERO      # latest bias-corrected rotation rate, °/s (body frame)
var a_dev := 0.0           # how far the acceleration is from its smoothed value, g
var poses: Array[Dictionary] = []  # recent {t (device ms), q}

var _ws := WebSocketPeer.new()
var _open := false
var _retry_ms := 500.0
var _retry_at := 0.0
var _has_q := false
var _has_offset := false
var _offset := 0.0         # local ms − device ms, from the fastest-arriving samples
var _last_seq := -1
var _btn := 0
var _last_t := 0.0
var _integ := Vector3.ZERO
var _bias := Vector3.ZERO
var _a_avg := Vector3(0, 0, 1)
var _g_avg := Vector3.ZERO
var _bias_still_ms := 0.0
var _bias_learned_ms := 0.0


static func now_ms() -> float:
	return Time.get_ticks_usec() / 1000.0


## World "up" in stick coordinates for orientation q
static func up_body(quat: Quaternion) -> Vector3:
	return quat.inverse() * Vector3(0, 0, 1)


func connect_to(new_host: String) -> void:
	host = new_host
	_ws.close()
	_ws = WebSocketPeer.new()
	_ws.inbound_buffer_size = 1 << 18
	_open = false
	connected = false
	_retry_ms = 500.0
	_retry_at = 0.0
	status = "Connecting to %s…" % host
	_ws.connect_to_url("ws://%s/ws" % host)


## Call every frame: reads what has arrived and reconnects with backoff
func poll() -> void:
	_ws.poll()
	match _ws.get_ready_state():
		WebSocketPeer.STATE_OPEN:
			if not _open:
				_open = true
				connected = true
				_retry_ms = 500.0
				_reset_stream()
				status = "Live"
			while _ws.get_available_packet_count() > 0:
				var msg: Variant = JSON.parse_string(_ws.get_packet().get_string_from_utf8())
				if msg is Dictionary and msg.has("s"):
					imu = str(msg.get("imu", ""))
					status = "Live · %s" % imu
					_feed_batch(msg["s"])
		WebSocketPeer.STATE_CLOSED:
			if _open or connected or _retry_at == 0.0:
				_open = false
				connected = false
				_retry_at = now_ms() + _retry_ms
				status = "No stick at %s — retrying" % host
				_retry_ms = minf(_retry_ms * 2.0, 8000.0)
			elif now_ms() >= _retry_at:
				_retry_at = 0.0
				_ws = WebSocketPeer.new()
				_ws.inbound_buffer_size = 1 << 18
				_ws.connect_to_url("ws://%s/ws" % host)


func ready() -> bool:
	return connected and _has_offset and not poses.is_empty()


## The stick's clock right now, a little behind the newest sample
func device_now() -> float:
	return now_ms() - LAG_MS - _offset


## Orientation at device time t, interpolated between samples
func quat_at(t: float) -> Quaternion:
	var n := poses.size()
	if t >= float(poses[n - 1]["t"]):
		return poses[n - 1]["q"]
	var j := n - 1
	while j > 0 and float(poses[j - 1]["t"]) > t:
		j -= 1
	if j == 0:
		return poses[0]["q"]
	var t0: float = poses[j - 1]["t"]
	var t1: float = poses[j]["t"]
	var q0: Quaternion = poses[j - 1]["q"]
	var q1: Quaternion = poses[j]["q"]
	return q0.slerp(q1, (t - t0) / (t1 - t0))


# On (re)connect: the stick may have rebooted, so its clock and sequence numbers restart
func _reset_stream() -> void:
	_has_q = false
	_integ = Vector3.ZERO
	_a_avg = Vector3(0, 0, 1)
	_bias_still_ms = 0.0
	w = Vector3.ZERO
	_last_seq = -1
	poses.clear()
	_has_offset = false


func _feed_batch(rows: Array) -> void:
	if rows.is_empty():
		return
	# A big batch is history (on connect) or a backlog after a stall: feed the filter, but don't
	# act on its buttons or motion
	var replay := rows.size() > 8
	var newest_t: float = rows[rows.size() - 1][1]
	var arrive := now_ms() - newest_t
	_offset = arrive if not _has_offset or arrive < _offset else _offset + (arrive - _offset) * 0.002
	_has_offset = true

	for row: Array in rows:
		var seq := int(row[0])
		if seq <= _last_seq:
			continue  # history can overlap the live stream
		_last_seq = seq
		var s := {
			"seq": seq, "t": float(row[1]),
			"g": Vector3(row[2], row[3], row[4]), "a": Vector3(row[5], row[6], row[7]),
			"b": int(row[8]) if row.size() > 8 else 0,
		}
		var dt := _update_orientation(s)
		poses.append({"t": s["t"], "q": q})
		var b: int = s["b"]
		var newly := b & ~_btn
		_btn = b
		if replay or dt == 0.0:
			continue
		if newly & 2:
			pressed.emit(2)
		if newly & 1:
			pressed.emit(1)
		sample.emit(s, dt)
	while poses.size() > 2 and float(poses[0]["t"]) < newest_t - 2000.0:
		poses.pop_front()


static func _quat_from_accel(a: Vector3) -> Quaternion:
	# Tilt from gravity, heading zero
	var roll := atan2(a.y, a.z)
	var pitch := atan2(-a.x, Vector2(a.y, a.z).length())
	var cr := cos(roll / 2.0)
	var sr := sin(roll / 2.0)
	var cp := cos(pitch / 2.0)
	var sp := sin(pitch / 2.0)
	return Quaternion(sr * cp, cr * sp, -sr * sp, cr * cp)


# Mahony filter. Returns the time step used (s), or 0 when the sample only initialized or was
# skipped.
func _update_orientation(s: Dictionary) -> float:
	var a: Vector3 = s["a"]
	var g: Vector3 = s["g"]
	var t: float = s["t"]
	if not _has_q:
		q = _quat_from_accel(a)
		_last_t = t
		_a_avg = a
		_g_avg = g
		_has_q = true
		return 0.0
	if t <= _last_t:
		return 0.0
	var dt := minf((t - _last_t) / 1000.0, 0.1)
	_last_t = t

	var a_mag := a.length()
	_a_avg += (a - _a_avg) * minf(1.0, dt / 0.5)
	_g_avg += (g - _g_avg) * minf(1.0, dt / 0.2)
	a_dev = (a - _a_avg).length()

	# Learn the gyro's offset while the stick is still (see web/motion.js for the reasoning)
	var learned := _bias_learned_ms > 1000.0
	var quiet := (_g_avg - _bias).length() < (2.5 if learned else 8.0) and a_dev < 0.03 and absf(a_mag - 1.0) < 0.08
	_bias_still_ms = _bias_still_ms + dt * 1000.0 if quiet else 0.0
	if _bias_still_ms > 400.0:
		_bias += (_g_avg - _bias) * minf(1.0, dt / (2.0 if learned else 0.3))
		_bias_learned_ms += dt * 1000.0
	w = g - _bias
	var rate := w * (PI / 180.0)

	# Correct toward the measured gravity direction; its weight fades out as |a| leaves 1 g
	var trust := maxf(0.0, 1.0 - absf(a_mag - 1.0) / 0.15)
	if trust > 0.0 and a_mag > 0.0:
		var e := (a / a_mag).cross(up_body(q))
		_integ += e * (trust * KI * dt)
		rate += e * (trust * KP) + _integ

	# Rotate by the whole step at once, q ⊗ exp(ω dt / 2)
	var wn := rate.length()
	if wn > 1e-9:
		var half := 0.5 * wn * dt
		var k := sin(half) / wn
		q = (q * Quaternion(rate.x * k, rate.y * k, rate.z * k, cos(half))).normalized()
	return dt
