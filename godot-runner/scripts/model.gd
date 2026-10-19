class_name RunnerModel
extends RefCounted
## The runner game's rules and motion detection, with nothing drawn: a port of the game model in
## web/runner.html. The stick sits in your trouser pocket; your runner jogs down the road on its
## own and faster the quicker you run on the spot, and you jump, squat and sidestep to get past
## obstacles.
##
## The road runs downhill into the screen: distance s along it maps to z = −s, its centre line
## winds (road_x) and drops (road_y). Lengths in m, speeds in m/s, times in ms.

const PACE := 7.0                    # the speed the levels' timings are quoted at (25 km/h)
const JOG := 5.0                     # speed while you stand still
const TOP := 10.0                    # speed running flat out
const SPM_MIN := 80.0                # steps per minute where the speed starts to rise
const SPM_TOP := 240.0               # ... and where it tops out
const SPEED_UP_S := 0.6              # how quickly the runner picks up speed
const COAST_S := 2.5                 # ... and loses it once you stop
const HALF_W := 4.0
const EDGE := 6.3
const STAND_H := 1.8                 # runner's height standing
const DUCK_H := 1.1                  # ... and crouched
const CLEARANCE := 1.3               # high obstacles leave this much room underneath
const JUMP_PEAK := 1.2               # an in-game jump always reaches this height
const R_RIDER := 0.3
const STEP_MS := 4.0

# Detection from the stick in your pocket; the reasoning behind each number is in web/runner.html.
# Jumps: a push-off above PUSH_G, then under AIR_G for AIR_SAMPLES samples; landing above LAND_G.
const PUSH_G := 1.3
const AIR_G := 0.5
const AIR_SAMPLES := 3
const LAND_G := 1.3
const RELEASE_DEG := 10.0
# Running on the spot: a stride is the thigh swinging forward past STRIDE_HI after being back
# under STRIDE_LO. While there's a rhythm, a squat has to outlast a knee lift (DUCK_DWELL of a
# stride) and a jump needs AIR_SAMPLES_RUN samples of free fall.
const STRIDE_HI := 12.0
const STRIDE_LO := 8.0
const STRIDE_MIN_MS := 300.0
const STRIDE_MAX_MS := 1100.0
const DUCK_DWELL := 0.5
const AIR_SAMPLES_RUN := 10
const FWD_DEG := 30.0                # which way is forward is learned from tilts past this: only squats go that far
# Sidesteps: a step is the thigh tilting sideways past SIDE_TILT while not far forward
# (SIDE_PITCH_MAX rules out squats and knee lifts); it counts again once the tilt is back under
# SIDE_REARM. Which way is the sign of your sideways speed just before: the sideways acceleration
# added up, leaking away over SIDE_LEAK_S, at its largest in the last SIDE_WIN_MS. Under
# SIDE_V_MIN it's too small to tell and the step is ignored.
const LANE_W := 2.0 * HALF_W / 3.0
const SIDE_TILT := 12.5
const SIDE_REARM := 6.0
const SIDE_PITCH_MAX := 20.0
const SIDE_V_MIN := 0.1
const SIDE_LEAK_S := 0.5
const SIDE_WIN_MS := 400.0
const SIDE_LOCK_MS := 600.0
const LANE_S := 0.12                 # how quickly the runner slides across to a new lane
# Coins fill the stretches between obstacles: a line along one lane, or an arc in the air to jump for
const COIN_POINTS := 10
const COIN_SPACING := 2.0
const COIN_REACH := 0.6
const COIN_ARC := 0.3                # share of groups that are arcs

# Levels. Times are at PACE; run faster and everything comes at you quicker:
#  gap    — seconds between obstacles; hold — how long high obstacles are, in seconds under them
#  air    — how long an in-game jump lasts, ms (it covers the same distance at any speed)
#  duck   — thigh angle that counts as squatting
#  combo  — chance an obstacle is followed ~1.2 s later by one of another kind
#  side   — share of obstacles that are barriers to step around; wide — chance one blocks two lanes
#  cue    — show JUMP! / DUCK! / LEFT! / RIGHT! as they come
const LEVELS := {
	"easy": {
		"label": "Easy", "desc": "Long stretches of coins between obstacles, a gentle squat is enough, a few one-lane barriers, cues for everything, 3 lives.",
		"gap": [4.6, 6.0], "hold": [0.3, 0.6], "air": 1000.0, "duck": 22.0, "combo": 0.0, "side": 0.25, "wide": 0.0, "cue": true, "lives": 3,
	},
	"medium": {
		"label": "Medium", "desc": "Obstacles come faster, some in pairs, long ones to stay down under, and barriers two lanes wide. 2 lives.",
		"gap": [3.4, 4.4], "hold": [0.3, 1.0], "air": 850.0, "duck": 28.0, "combo": 0.25, "side": 0.33, "wide": 0.3, "cue": true, "lives": 2,
	},
	"hard": {
		"label": "Hard", "desc": "Quick pairs, deeper squats, long tunnels to hold, wide barriers, tighter jump timing and no cues. One life.",
		"gap": [2.5, 3.3], "hold": [0.5, 1.5], "air": 750.0, "duck": 33.0, "combo": 0.5, "side": 0.4, "wide": 0.5, "cue": false, "lives": 1,
	},
}
const LOW_TYPES := [["log", 0.4, 0.45], ["hurdle", 0.55, 0.25], ["crates", 0.5, 0.6]]  # type, height, length

var stick: Stick
var level := "easy"
var best := {}                       # best score per level

# The level in play
var lv_gap := Vector2(3.2, 4.2)
var lv_hold := Vector2(0.3, 0.6)
var lv_air := 1000.0
var lv_duck := 22.0
var lv_combo := 0.0
var lv_side := 0.25
var lv_wide := 0.0
var lv_cue := true
var lv_lives := 3

var phase := "menu"                  # "menu" | "calibrate" (stand still, then squat once) | "play" | "over"
var paused := false
var clock := 0.0                     # game time, ms
var sim_t := 0.0                     # the stick's time the game has been advanced to
var has_sim := false
var up0 := Vector3.UP                # "up" in stick coordinates while you stood still: your standing thigh
var has_up0 := false
var s := 0.0                         # distance along the road
var v := JOG
var top_v := JOG
var h := 0.0                         # height off the ground
var vh := 0.0
var jump_g := 0.0
var air := false
var ducking := false
var angle := 0.0                     # thigh tilt from standing, degrees
var lane := 0                        # −1 left, 0 middle, 1 right
var x := 0.0                         # metres from the centre line
var lives := 3
var cleared := 0
var coins := 0
var bonus := 0
var jumps := 0
var squats := 0
var best_jump := 0
var crash_until := 0.0
var safe_until := 0.0
var next_s := 0.0
var takeoffs: Array[float] = []      # device times of detected real jumps, for the simulation to act on
var obstacles: Array[Dictionary] = []  # {id, kind: "low"|"high"|"side", type, s, d, h, lanes, cued, passed, hit, ghost}
var coin_list: Array[Dictionary] = []  # {id, s, lane, h (height of its centre), air (only reachable in a jump), taken}
var events: Array[Dictionary] = []

var _calib_end := 0.0
var _calib_sum := Vector3.ZERO
var _tilted_at := -1.0
var _duck_since := 0.0
var _duck_counts := false
var _still_duck := 0.0
var _next_id := 0

# Real jump detection
var _peak_at := -1e9
var _low := 0
var _below1_at := 0.0
var _rise_at := 0.0
var _prev_g := 1.0
var _in_air := false
var _takeoff_at := 0.0
var _land_at := -1e9

# Running on the spot. run_fwd is the way your thigh tilts forward, in stick coordinates.
var run_fwd := Vector3.ZERO
var has_fwd := false
var run_spm := 0.0
var run_streak := 0
var run_steps := 0
var _run_armed := false
var _run_last_at := -1e9
var _run_period := 0.0

# Sidesteps: sideways speed in m/s and the thigh's sideways tilt in degrees, positive to your left
var side_v := 0.0
var side_roll := 0.0
var _side_recent: Array[Vector2] = []  # (device time, sideways speed) over the last SIDE_WIN_MS
var _side_armed := true
var _side_lock_until := 0.0


static func road_x(d: float) -> float:
	return 18.0 * sin(d / 140.0) + 6.0 * sin(d / 67.0 + 1.3)


static func road_dx(d: float) -> float:
	return 18.0 / 140.0 * cos(d / 140.0) + 6.0 / 67.0 * cos(d / 67.0 + 1.3)


static func road_y(d: float) -> float:
	return -0.05 * d + 1.0 * sin(d / 45.0) + 0.4 * sin(d / 19.0 + 0.7)


## The point `d` metres down the road, `across` metres right of its centre line, `up` above it
static func at(d: float, across: float, up := 0.0) -> Vector3:
	return Vector3(road_x(d) + across, road_y(d) + up, -d)


func _init(the_stick: Stick) -> void:
	stick = the_stick
	stick.sample.connect(_on_sample)


func run_score() -> int:
	return floori(s) + bonus


func start_game() -> void:
	var lv: Dictionary = LEVELS[level]
	lv_gap = Vector2(lv["gap"][0], lv["gap"][1])
	lv_hold = Vector2(lv["hold"][0], lv["hold"][1])
	lv_air = lv["air"]
	lv_duck = lv["duck"]
	lv_combo = lv["combo"]
	lv_side = lv["side"]
	lv_wide = lv["wide"]
	lv_cue = lv["cue"]
	lv_lives = lv["lives"]

	phase = "calibrate"
	paused = false
	_calib_end = clock + 2000.0
	_calib_sum = Vector3.ZERO
	has_up0 = false
	s = 0.0
	v = JOG
	top_v = JOG
	h = 0.0
	vh = 0.0
	air = false
	lane = 0
	x = 0.0
	ducking = false
	_tilted_at = -1.0
	_still_duck = 0.0
	lives = lv_lives
	cleared = 0
	coins = 0
	bonus = 0
	jumps = 0
	squats = 0
	best_jump = 0
	crash_until = 0.0
	safe_until = 0.0
	next_s = 5.0 * PACE
	takeoffs.clear()
	obstacles.clear()
	coin_list.clear()
	side_v = 0.0
	side_roll = 0.0
	_side_recent.clear()
	_side_armed = true
	_side_lock_until = 0.0
	has_fwd = false
	run_spm = 0.0
	run_streak = 0
	run_steps = 0
	_run_armed = false
	_run_last_at = -1e9
	_run_period = 0.0
	events.append({"type": "start"})


func show_menu() -> void:
	phase = "menu"
	paused = false
	obstacles.clear()
	coin_list.clear()


func toggle_pause() -> void:
	if phase != "play" and phase != "calibrate":
		return
	paused = not paused
	events.append({"type": "paused" if paused else "resumed"})


# ---- Runs on every live sample ----

func _on_sample(sm: Dictionary, dt: float) -> void:
	var a: Vector3 = sm["a"]
	var t: float = sm["t"]
	# Takeoff: under AIR_G for long enough within 400 ms of a push-off. Air time runs from dropping
	# below 1 g to coming back above it just before the landing spike.
	var g := a.length()
	if g > PUSH_G:
		_peak_at = t
	if g < 1.0 and _prev_g >= 1.0:
		_below1_at = t
	if g >= 1.0 and _prev_g < 1.0:
		_rise_at = t
	_prev_g = g
	if not _in_air:
		_low = _low + 1 if g < AIR_G else 0
		if _low >= (AIR_SAMPLES_RUN if run_spm > 0.0 else AIR_SAMPLES) and t - _peak_at < 400.0:
			_in_air = true
			_takeoff_at = _below1_at if _below1_at > 0.0 else t
			takeoffs.append(t)
	elif g > LAND_G or t - _takeoff_at > 1000.0:
		var land_at := _rise_at if t - _rise_at < 60.0 else t
		var air_s := (land_at - _takeoff_at) / 1000.0
		_in_air = false
		_low = 0
		_peak_at = -1e9
		_land_at = t
		if air_s < 1.0:
			events.append({"type": "real_jump", "cm": roundi(9.81 * air_s * air_s / 8.0 * 100.0)})
	_track_run(t)
	_track_side(a, t, dt)


# The thigh's swing is its tilt from standing, signed: forward (the way squats tilt it, learned
# from the big tilts) is positive, so the back-swing isn't counted as a second
# stride. One stride of the pocket leg is two steps.
func _track_run(t: float) -> void:
	if not has_up0:
		return
	if run_spm > 0.0 and t - _run_last_at > minf(1300.0, 1.6 * _run_period):
		run_spm = 0.0  # the rhythm stopped
		run_streak = 0
	var up := Stick.up_body(stick.q)
	var c := clampf(up.dot(up0), -1.0, 1.0)
	var tilt_deg := rad_to_deg(acos(c))
	var tilt := up - up0 * c
	if tilt_deg > FWD_DEG:
		run_fwd = run_fwd.lerp(tilt.normalized(), 0.05).normalized() if has_fwd else tilt.normalized()
		has_fwd = true
	var swing := -tilt_deg if has_fwd and tilt.dot(run_fwd) < 0.0 else tilt_deg
	if swing < STRIDE_LO:
		_run_armed = true
	if not _run_armed or swing < STRIDE_HI:
		return
	_run_armed = false
	var period := t - _run_last_at
	_run_last_at = t
	if period < STRIDE_MIN_MS or period > STRIDE_MAX_MS:
		return
	run_spm = (run_spm + 120000.0 / period) / 2.0 if run_spm > 0.0 else 120000.0 / period
	_run_period = period
	run_streak += 1
	if phase == "play" and not paused:
		run_steps += 2


# Your left, in stick coordinates, is square to "up" and to the way your thigh tilts forward, so
# it holds however the stick sits in the pocket and whichever way you turn. The thigh's tilt from
# standing splits into forward (pitch) and sideways (roll); the acceleration to your left, with
# gravity taken out, adds up to a sideways speed.
func _track_side(a: Vector3, t: float, dt: float) -> void:
	if not has_up0 or not has_fwd:
		return
	var up := Stick.up_body(stick.q)
	var left0 := up0.cross(run_fwd).normalized()
	var left := left0 - up * left0.dot(up)  # level, even with the thigh tilted
	var n := left.length()
	if n < 0.5:
		return
	side_v = (side_v + (a - up).dot(left) / n * 9.81 * dt) * exp(-dt / SIDE_LEAK_S)
	_side_recent.append(Vector2(t, side_v))
	while _side_recent[0].x < t - SIDE_WIN_MS:
		_side_recent.pop_front()

	var tilt := up - up0 * up.dot(up0)
	side_roll = rad_to_deg(asin(clampf(tilt.dot(left0), -1.0, 1.0)))
	var pitch := rad_to_deg(asin(clampf(tilt.dot(run_fwd), -1.0, 1.0)))
	if absf(side_roll) < SIDE_REARM:
		_side_armed = true
	if not _side_armed or absf(side_roll) < SIDE_TILT or absf(pitch) > SIDE_PITCH_MAX or t < _side_lock_until:
		return
	_side_armed = false
	var speed := 0.0
	for r in _side_recent:
		if absf(r.y) > absf(speed):
			speed = r.y
	if absf(speed) < SIDE_V_MIN or _in_air or t - _land_at < 250.0:
		return  # a landing shakes the pocket sideways too
	_side_lock_until = t + SIDE_LOCK_MS
	if phase != "play" or paused or clock < crash_until:
		return
	var new_lane := clampi(lane - (1 if speed > 0.0 else -1), -1, 1)
	if new_lane != lane:
		lane = new_lane
		events.append({"type": "lane"})


# How far "up" in stick coordinates has tilted from where it was while you stood still. Once
# forward is known, only the forward part counts, so the sideways tilt of a sidestep isn't taken
# for a squat.
func _thigh_angle(q: Quaternion) -> float:
	if not has_up0:
		return 0.0
	var up := Stick.up_body(q)
	var c := clampf(up.dot(up0), -1.0, 1.0)
	if not has_fwd:
		return rad_to_deg(acos(c))
	return maxf(0.0, rad_to_deg(asin(clampf((up - up0 * c).dot(run_fwd), -1.0, 1.0))))


# ---- Obstacles ----

func _add(kind: String, type: String, at_s: float, d: float, height: float, lanes: Array) -> void:
	_next_id += 1
	obstacles.append({
		"id": _next_id, "kind": kind, "type": type, "s": at_s, "d": d, "h": height, "lanes": lanes,
		"cued": false, "passed": false, "hit": false, "ghost": false,
	})


func _spawn(at_s: float, kind: String) -> float:
	if kind == "side":
		# Two lanes wide leaves one lane open; one lane wide is in the middle half the time
		var open := randi_range(-1, 1)
		var lanes: Array = []
		if randf() < lv_wide:
			for l in [-1, 0, 1]:
				if l != open:
					lanes.append(l)
		else:
			lanes.append(0 if randf() < 0.5 else randi_range(-1, 1))
		_add(kind, "wall", at_s, 0.5, 0.0, lanes)
		return 0.5
	if kind == "low":
		var low: Array = LOW_TYPES[randi() % LOW_TYPES.size()]
		_add(kind, low[0], at_s, low[2], low[1], [])
		return low[2]
	var d := maxf(0.4, randf_range(lv_hold.x, lv_hold.y) * PACE)
	_add(kind, "tunnel" if d > 4.0 else ("banner" if d > 2.0 else "bar"), at_s + d / 2.0, d, 0.0, [])
	return d


func _pick(not_kind: String) -> String:
	while true:
		var k := "side" if randf() < lv_side else ("low" if randf() < 0.5 else "high")
		if k != not_kind:
			return k
	return "low"


# A group of coins in one lane, in the `room` metres of clear road from at_s
func _place_coins(at_s: float, room: float) -> void:
	if room < 4.0:
		return
	var coin_lane := randi_range(-1, 1)
	if randf() < COIN_ARC:
		for i in 3:
			_next_id += 1
			coin_list.append({"id": _next_id, "s": at_s + room / 2.0 + (i - 1) * 1.2, "lane": coin_lane, "h": 1.9 if i == 1 else 1.6, "air": true, "taken": false})
		return
	for i in mini(6, floori(room / COIN_SPACING) + 1):
		_next_id += 1
		coin_list.append({"id": _next_id, "s": at_s + i * COIN_SPACING, "lane": coin_lane, "h": 0.9, "air": false, "taken": false})


func _fill_road() -> void:
	while next_s < s + 130.0:
		var kind := _pick("")
		var end := next_s + _spawn(next_s, kind)
		if randf() < lv_combo:
			end += _spawn(end + 1.2 * PACE, _pick(kind)) + 1.2 * PACE
		var gap := randf_range(lv_gap.x, lv_gap.y) * PACE
		_place_coins(end + 5.0, gap - 10.0)  # clear of the obstacles at either end
		next_s = end + gap
	for i in range(obstacles.size() - 1, -1, -1):
		if float(obstacles[i]["s"]) < s - 20.0:
			obstacles.remove_at(i)
	for i in range(coin_list.size() - 1, -1, -1):
		if float(coin_list[i]["s"]) < s - 20.0:
			coin_list.remove_at(i)


# ---- Running the road ----

func _crash(o: Dictionary) -> void:
	o["hit"] = true
	lives -= 1
	crash_until = clock + 1200.0
	safe_until = clock + 2600.0
	air = false
	h = 0.0
	vh = 0.0
	v = JOG
	events.append({"type": "crash", "kind": o["kind"]})
	if lives > 0:
		return
	phase = "over"
	var score := run_score()
	var is_best := score > int(best.get(level, 0))
	if is_best:
		best[level] = score
	events.append({"type": "over", "score": score, "is_best": is_best})


func _step(q: Quaternion, step_ms: float) -> void:
	var dt := step_ms / 1000.0
	angle = _thigh_angle(q)
	# Squatting, with a little hysteresis so it doesn't flicker at the threshold. It counts as a
	# squat once held 0.3 s, unless it began just after a landing (knees absorbing the jump).
	# While you're running it only starts once the thigh has stayed up longer than a knee lift does.
	var was := ducking
	var tilted := angle > (lv_duck - RELEASE_DEG if was else lv_duck)
	if not tilted:
		_tilted_at = -1.0
	elif _tilted_at < 0.0:
		_tilted_at = clock
	ducking = tilted and (was or run_spm == 0.0 or clock - _tilted_at >= DUCK_DWELL * _run_period)
	if ducking and not was:
		events.append({"type": "duck"})
		_duck_since = clock
		_duck_counts = sim_t - _land_at > 400.0
	if ducking and _duck_counts and clock - _duck_since >= 300.0:
		_duck_counts = false
		squats += 1

	# The stick can shift in the pocket. While you stand still (not squatting) the standing
	# reference slowly follows it, and a moderate "squat" held dead still for 3 s — longer than
	# any obstacle needs — is taken as the stick having moved: that becomes the new standing.
	if has_up0:
		var still := stick.w.length() < 25.0 and stick.a_dev < 0.05
		var up := Stick.up_body(q)
		if not ducking and still:
			up0 = up0.lerp(up, minf(1.0, dt / 6.0)).normalized()
		_still_duck = _still_duck + step_ms if ducking and still and angle < 55.0 else 0.0  # sitting (70°+) isn't a shift
		if _still_duck > 3000.0:
			up0 = up
			_still_duck = 0.0
			ducking = false
			events.append({"type": "recal"})

	# Calibration: stand still to learn how the stick sits, then one squat to learn which way is
	# forward (and so which way is left)
	if phase == "calibrate":
		if not has_up0:
			_calib_sum += Stick.up_body(q)
			if clock >= _calib_end:
				up0 = _calib_sum.normalized()
				has_up0 = true
				events.append({"type": "squat_once"})
		elif has_fwd and angle < 10.0:
			phase = "play"
			ducking = false
			takeoffs.clear()
			side_v = 0.0
			_side_recent.clear()
			events.append({"type": "go"})
		return

	# In-game jumps start the moment a real one is detected
	while not takeoffs.is_empty() and takeoffs[0] <= sim_t:
		takeoffs.pop_front()
		jumps += 1
		if not air and clock >= crash_until:
			var T := lv_air / 1000.0 * PACE / v
			jump_g = 8.0 * JUMP_PEAK / (T * T)
			air = true
			vh = jump_g * T / 2.0
			events.append({"type": "jump"})
	if air:
		h += vh * dt - jump_g * dt * dt / 2.0
		vh -= jump_g * dt
		if h <= 0.0:
			air = false
			h = 0.0
			vh = 0.0
			events.append({"type": "land"})
	if clock < crash_until:
		return
	# Speed follows your cadence: quick to pick up, slow to ease off, so stopping to jump or
	# squat doesn't throw it away
	var cadence := run_spm if run_streak > 1 else 0.0
	var target := JOG + (TOP - JOG) * clampf((cadence - SPM_MIN) / (SPM_TOP - SPM_MIN), 0.0, 1.0)
	v += (target - v) * minf(1.0, dt / (SPEED_UP_S if target > v else COAST_S))
	top_v = maxf(top_v, v)
	s += v * dt

	x += (lane * LANE_W - x) * minf(1.0, dt / LANE_S)

	# Coins: run through the ones in your lane; the ones in the air need a jump
	for c in coin_list:
		if c["taken"] or absf(float(c["s"]) - s) > COIN_REACH or absf(x - int(c["lane"]) * LANE_W) > LANE_W / 2.0:
			continue
		if c["air"] and h < 0.4:
			continue
		c["taken"] = true
		coins += 1
		bonus += COIN_POINTS
		events.append({"type": "coin"})

	# Obstacles: low ones must pass under your feet, high ones over your head, barriers beside you
	var top := h + (DUCK_H if ducking and not air else STAND_H)
	for o in obstacles:
		if o["hit"] or o["passed"]:
			continue
		var kind: String = o["kind"]
		var o_s: float = o["s"]
		var o_d: float = o["d"]
		var lanes: Array = o["lanes"]
		var lead := (o_s - o_d / 2.0 - R_RIDER - s) / v  # seconds until you reach it
		if lv_cue and not o["cued"] and lead < (0.6 if kind == "low" else 1.0):
			o["cued"] = true
			if kind != "side":
				events.append({"type": "cue", "kind": kind, "dir": 0})
			elif lanes.has(lane):
				# Point to the nearest open lane; from the middle with both sides open, either will do
				var near := 9
				var to: Array[int] = []
				for l in [-1, 0, 1]:
					if lanes.has(l):
						continue
					var dist := absi(l - lane)
					if dist < near:
						near = dist
						to.clear()
					if dist == near:
						to.append(l)
				events.append({"type": "cue", "kind": kind, "dir": 0 if to.size() > 1 else signi(to[0] - lane)})
		if s > o_s + o_d / 2.0 + R_RIDER:
			o["passed"] = true
			if not o["ghost"]:
				cleared += 1
				bonus += roundi(20.0 * v / PACE)
				events.append({"type": "cleared"})
			continue
		if lead > 0.0:
			continue
		if clock < safe_until:
			o["ghost"] = true  # passing through right after a crash doesn't count
			continue
		var blocked := false
		if kind == "side":
			for l: int in lanes:
				if absf(x - l * LANE_W) < LANE_W / 2.0:
					blocked = true
		elif kind == "low":
			blocked = h < float(o["h"])
		else:
			blocked = top > CLEARANCE
		if blocked:
			_crash(o)
			return


## Advance to device time `target` in small steps against the interpolated stick
func simulate(target: float) -> void:
	if not has_sim or target - sim_t > 250.0 or target < sim_t - 1000.0 or paused or phase == "menu":
		sim_t = target
		has_sim = true
		takeoffs.clear()
		return
	while sim_t < target:
		var step_ms := minf(STEP_MS, target - sim_t)
		sim_t += step_ms
		clock += step_ms
		if phase == "play" or phase == "calibrate":
			_step(stick.quat_at(sim_t), step_ms)
		else:
			angle = _thigh_angle(stick.quat_at(sim_t))
	if phase == "play":
		_fill_road()
