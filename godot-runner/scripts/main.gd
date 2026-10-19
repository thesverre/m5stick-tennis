extends Node3D
## The runner game's scene: road, obstacles, runner, camera and the menus, all built in code and
## driven by RunnerModel. A Godot port of web/runner.html; it talks to the stick over WiFi.

const M := preload("res://scripts/model.gd")
const SETTINGS_PATH := "user://runner.cfg"
const CHUNK := 40.0
const ROAD_W := 2.0 * M.HALF_W + 0.6  # obstacles reach a little past the road's edges
const SKY := Color("a9d3f2")
const ORANGE := Color("f07a1a")
const BLUE := Color("2a78d6")
const RED := Color("c2255c")


# The squat gauge and the sidestep indicator, drawn by hand
class Meters:
	extends Control
	const SIDE_FULL := 25.0  # sideways tilt, in degrees, at either end of the sidestep indicator
	var angle := 0.0
	var duck := 22.0
	var ducking := false
	var side_roll := 0.0
	var lane := 0

	func _draw() -> void:
		var font := ThemeDB.fallback_font
		var panel := Color(0.1, 0.1, 0.1, 0.8)
		var track := Color(1, 1, 1, 0.15)
		var grey := Color(0.6, 0.6, 0.58)
		# Squat: how far your thigh is tilted, with a mark where it counts as ducking
		draw_rect(Rect2(0, 0, 70, 190), panel)
		draw_string(font, Vector2(0, 20), "SQUAT", HORIZONTAL_ALIGNMENT_CENTER, 70, 12, grey)
		var full := maxf(60.0, duck * 2.0)
		draw_rect(Rect2(28, 30, 14, 120), track)
		var fill := clampf(angle / full, 0.0, 1.0) * 120.0
		draw_rect(Rect2(28, 150 - fill, 14, fill), BLUE if ducking else grey)
		draw_rect(Rect2(24, 150 - duck / full * 120.0 - 1, 22, 2), Color.WHITE)
		draw_string(font, Vector2(0, 176), "%d°" % roundi(angle), HORIZONTAL_ALIGNMENT_CENTER, 70, 15)
		# Sidestep: how far your thigh is tilted sideways, with marks where it counts as a step
		draw_rect(Rect2(80, 110, 160, 80), panel)
		draw_string(font, Vector2(80, 130), "SIDESTEP", HORIZONTAL_ALIGNMENT_CENTER, 160, 12, grey)
		draw_rect(Rect2(100, 140, 120, 14), track)
		for sgn in [-1.0, 1.0]:
			draw_rect(Rect2(160 + sgn * M.SIDE_TILT / SIDE_FULL * 60.0 - 1, 136, 2, 22), Color.WHITE)
		var knob := 160.0 - clampf(side_roll / SIDE_FULL, -1.0, 1.0) * 60.0
		draw_circle(Vector2(knob, 147), 6, RED if lane != 0 else grey)
		draw_string(font, Vector2(80, 178), ["Left", "Middle", "Right"][lane + 1], HORIZONTAL_ALIGNMENT_CENTER, 160, 15)


var stick := Stick.new()
var game: RunnerModel
var probe := false  # `-- --probe` on the command line: print what the stick sends and start a run without calibrating, for checking the link and the scene

var cam: Camera3D
var sun: DirectionalLight3D
var chunks := {}          # chunk index → Node3D
var obstacle_nodes := {}  # obstacle id → Node3D
var coin_nodes := {}      # coin id → MeshInstance3D
var coin_mesh: CylinderMesh
var mats := {}

var runner_root: Node3D
var rider: Node3D
var upper: Node3D
var leg_l: Node3D
var leg_r: Node3D
var arm_l: Node3D
var arm_r: Node3D
var crouch := 0.0
var stride_phase := 0.0   # where the legs are in their swing, radians
var cam_pos := Vector3.ZERO
var cam_set := false

var status_label: Label
var hud: Label
var meters: Meters
var banner_label: Label
var banner_small: Label
var banner_until := 0.0
var pop_label: Label
var pop_until := 0.0
var menu: Control
var over: Control
var over_summary: Label
var bar: Control
var pause_button: Button
var level_buttons := {}
var level_desc: Label
var host_edit: LineEdit
var _probe_at := 0.0
var _probe_samples := 0


func _ready() -> void:
	probe = OS.get_cmdline_user_args().has("--probe")
	game = RunnerModel.new(stick)
	stick.pressed.connect(func(bit: int) -> void:
		if bit == 1:
			_on_start_key())
	if probe:
		stick.sample.connect(func(_s: Dictionary, _dt: float) -> void: _probe_samples += 1)
	_load_settings()
	_build_world()
	_build_runner()
	_build_ui()
	_render_menu()
	stick.connect_to(stick.host)
	if probe:
		game.start_game()


# ---- Settings: level, stick address and best scores, kept between runs ----

func _load_settings() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(SETTINGS_PATH) != OK:
		return
	var lvl := str(cfg.get_value("runner", "level", "easy"))
	if M.LEVELS.has(lvl):
		game.level = lvl
	stick.host = str(cfg.get_value("runner", "host", stick.host))
	var saved: Variant = cfg.get_value("runner", "best", {})
	if saved is Dictionary:
		game.best = saved


func _save_settings() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("runner", "level", game.level)
	cfg.set_value("runner", "host", stick.host)
	cfg.set_value("runner", "best", game.best)
	cfg.save(SETTINGS_PATH)


# ---- World ----

func _mat(color: Color, rough := 0.8) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.roughness = rough
	return m


func _build_world() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = SKY
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color("dcebf7")
	env.ambient_light_energy = 0.45
	env.fog_enabled = true
	env.fog_mode = Environment.FOG_MODE_DEPTH
	env.fog_light_color = SKY
	env.fog_depth_begin = 50.0
	env.fog_depth_end = 130.0
	var world := WorldEnvironment.new()
	world.environment = env
	add_child(world)

	sun = DirectionalLight3D.new()
	sun.light_color = Color("fff4e0")
	sun.light_energy = 0.85
	sun.shadow_enabled = true
	sun.directional_shadow_max_distance = 60.0
	add_child(sun)
	sun.look_at_from_position(Vector3(6, 14, 4), Vector3.ZERO)

	cam = Camera3D.new()
	cam.fov = 60.0
	cam.near = 0.1
	cam.far = 400.0
	add_child(cam)

	mats = {
		"asphalt": _mat(Color("3b3e44")), "grass": _mat(Color("5e9e4c")), "field": _mat(Color("4f8a41")),
		"paint": _mat(Color("f2f2ea")), "trunk": _mat(Color("6b4a2f")), "leaf": _mat(Color("2f6e3a")),
		"grey": _mat(Color("9aa0a6")), "wood": _mat(Color("7a5232")), "orange": _mat(ORANGE, 0.7),
		"blue": _mat(BLUE, 0.6), "navy": _mat(Color("1f5aa6")), "red": _mat(RED, 0.6), "white": _mat(Color("f2f2ea")),
		"skin": _mat(Color("d8cbbb")), "shirt": _mat(BLUE), "pants": _mat(Color("2c3140")), "cap": _mat(ORANGE, 0.4),
	}
	var gold := _mat(Color("f5c518"), 0.3)
	gold.metallic = 0.5
	gold.emission_enabled = true
	gold.emission = Color("5a4400")
	coin_mesh = CylinderMesh.new()
	coin_mesh.top_radius = 0.26
	coin_mesh.bottom_radius = 0.26
	coin_mesh.height = 0.06
	coin_mesh.radial_segments = 20
	coin_mesh.material = gold
	for key in ["asphalt", "grass", "field", "paint"]:
		(mats[key] as StandardMaterial3D).cull_mode = BaseMaterial3D.CULL_DISABLED


# One strip of ground between `xa` and `xb` across the road, from s0 to s1 along it. With `dash`
# only every other stretch of that length is drawn.
func _strip(mesh: ArrayMesh, s0: float, s1: float, xa: float, xb: float, dy: float, material: Material, dash := 0.0) -> void:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	st.set_material(material)
	var rows := int(s1 - s0)
	for i in rows + 1:
		var d := s0 + i
		st.set_normal(Vector3.UP)
		st.add_vertex(M.at(d, xa, dy))
		st.set_normal(Vector3.UP)
		st.add_vertex(M.at(d, xb, dy))
		if i < rows and (dash == 0.0 or int(floor(d / dash)) % 2 == 0):
			for k in [0, 2, 1, 1, 2, 3]:
				st.add_index(2 * i + k)
	st.commit(mesh)


# The road is built in 40 m chunks ahead of you and dropped behind
func _build_chunk(k: int) -> Node3D:
	var s0 := k * CHUNK
	var s1 := s0 + CHUNK
	var group := Node3D.new()
	var mesh := ArrayMesh.new()
	_strip(mesh, s0, s1, -M.HALF_W, M.HALF_W, 0.0, mats["asphalt"])
	_strip(mesh, s0, s1, M.HALF_W, M.EDGE, -0.03, mats["grass"])
	_strip(mesh, s0, s1, -M.EDGE, -M.HALF_W, -0.03, mats["grass"])
	_strip(mesh, s0, s1, M.EDGE, 60.0, -0.3, mats["field"])
	_strip(mesh, s0, s1, -60.0, -M.EDGE, -0.3, mats["field"])
	for lx in [-M.LANE_W / 2.0, M.LANE_W / 2.0]:
		_strip(mesh, s0, s1, lx - 0.06, lx + 0.06, 0.01, mats["paint"], 3.0)
	_strip(mesh, s0, s1, M.HALF_W - 0.3, M.HALF_W - 0.18, 0.01, mats["paint"])
	_strip(mesh, s0, s1, -M.HALF_W + 0.18, -M.HALF_W + 0.3, 0.01, mats["paint"])
	var ground := MeshInstance3D.new()
	ground.mesh = mesh
	ground.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	group.add_child(ground)

	for i in 14:
		var d := randf_range(s0, s1)
		var across := (-1.0 if randf() < 0.5 else 1.0) * randf_range(M.EDGE + 2.0, 34.0)
		var sc := randf_range(0.8, 1.5)
		var tree := Node3D.new()
		var trunk := CylinderMesh.new()
		trunk.top_radius = 0.15
		trunk.bottom_radius = 0.2
		trunk.height = 1.6
		trunk.radial_segments = 6
		tree.add_child(_part(trunk, mats["trunk"], Vector3(0, 0.8, 0)))
		var leaves := CylinderMesh.new()
		leaves.top_radius = 0.0
		leaves.bottom_radius = 1.3
		leaves.height = 3.4
		leaves.radial_segments = 8
		tree.add_child(_part(leaves, mats["leaf"], Vector3(0, 3.0, 0)))
		tree.position = M.at(d, across, -0.3)
		tree.scale = Vector3.ONE * sc
		group.add_child(tree)
	add_child(group)
	return group


func _update_chunks() -> void:
	var first := floori((game.s - 30.0) / CHUNK)
	var last := floori((game.s + 140.0) / CHUNK)
	for k in range(first, last + 1):
		if not chunks.has(k):
			chunks[k] = _build_chunk(k)
	for k: int in chunks.keys():
		if k < first or k > last:
			(chunks[k] as Node3D).queue_free()
			chunks.erase(k)


func _part(mesh: PrimitiveMesh, material: Material, pos: Vector3) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	mesh.material = material
	mi.mesh = mesh
	mi.position = pos
	return mi


func _box(size: Vector3, material: Material, pos: Vector3) -> MeshInstance3D:
	var mesh := BoxMesh.new()
	mesh.size = size
	return _part(mesh, material, pos)


func _post(across: float, height: float, material: Material) -> MeshInstance3D:
	return _box(Vector3(0.16, height, 0.16), material, Vector3(across, height / 2.0, 0))


# Obstacles: orange = low (jump), blue = high (squat), red = barrier (step aside)
func _make_obstacle(o: Dictionary) -> Node3D:
	var g := Node3D.new()
	var type: String = o["type"]
	var d: float = o["d"]
	var height: float = o["h"]
	if type == "log":
		var log_mesh := CylinderMesh.new()
		log_mesh.top_radius = 0.2
		log_mesh.bottom_radius = 0.2
		log_mesh.height = ROAD_W
		log_mesh.radial_segments = 14
		var log_part := _part(log_mesh, mats["wood"], Vector3(0, 0.2, 0))
		log_part.rotation.z = PI / 2.0
		g.add_child(log_part)
		# Orange bands, so it reads as "jump" like the others
		for across in [-ROAD_W * 0.3, 0.0, ROAD_W * 0.3]:
			var band := CylinderMesh.new()
			band.top_radius = 0.21
			band.bottom_radius = 0.21
			band.height = 0.5
			var band_part := _part(band, mats["orange"], Vector3(across, 0.2, 0))
			band_part.rotation.z = PI / 2.0
			g.add_child(band_part)
	elif type == "hurdle":
		g.add_child(_box(Vector3(ROAD_W, 0.18, 0.08), mats["orange"], Vector3(0, height - 0.09, 0)))
		for across in [-ROAD_W / 2.0, 0.0, ROAD_W / 2.0]:
			g.add_child(_post(across, height, mats["grey"]))
	elif type == "crates":
		var across := -ROAD_W / 2.0 + 0.35
		while across < ROAD_W / 2.0:
			g.add_child(_box(Vector3(0.66, height, d), mats["orange"], Vector3(across, height / 2.0, 0)))
			across += 0.72
	elif type == "wall":
		# Barrier: a wall across each blocked lane, too tall to jump
		for l: int in o["lanes"]:
			g.add_child(_box(Vector3(M.LANE_W - 0.2, 2.1, d), mats["red"], Vector3(l * M.LANE_W, 1.05, 0)))
			g.add_child(_box(Vector3(M.LANE_W - 0.2, 0.25, d + 0.04), mats["white"], Vector3(l * M.LANE_W, 1.3, 0)))
	else:
		# High: a beam (bar), a banner, or a tunnel roof, all leaving CLEARANCE underneath
		var under := M.CLEARANCE + 0.02
		var thick := 0.3 if type == "bar" else (0.9 if type == "banner" else 1.6)
		g.add_child(_box(Vector3(ROAD_W, thick, d), mats["blue"], Vector3(0, under + thick / 2.0, 0)))
		var post_h := under + thick
		if type == "tunnel":
			for across in [-ROAD_W / 2.0, ROAD_W / 2.0]:
				g.add_child(_box(Vector3(0.3, post_h, d), mats["navy"], Vector3(across, post_h / 2.0, 0)))
		else:
			g.add_child(_post(-ROAD_W / 2.0, post_h, mats["grey"]))
			g.add_child(_post(ROAD_W / 2.0, post_h, mats["grey"]))
	add_child(g)
	return g


func _update_obstacles() -> void:
	var seen := {}
	for o in game.obstacles:
		var id: int = o["id"]
		seen[id] = true
		if not obstacle_nodes.has(id):
			obstacle_nodes[id] = _make_obstacle(o)
		var node: Node3D = obstacle_nodes[id]
		var o_s: float = o["s"]
		node.position = M.at(o_s, 0.0)
		node.rotation.y = -atan(M.road_dx(o_s))
		node.visible = not o["hit"]
	for id: int in obstacle_nodes.keys():
		if not seen.has(id):
			(obstacle_nodes[id] as Node3D).queue_free()
			obstacle_nodes.erase(id)


# Coins: gold discs on edge, turning about the vertical
func _update_coins() -> void:
	var seen := {}
	var spin := Time.get_ticks_msec() / 300.0
	for c in game.coin_list:
		var id: int = c["id"]
		seen[id] = true
		if not coin_nodes.has(id):
			var mi := MeshInstance3D.new()
			mi.mesh = coin_mesh
			mi.position = M.at(c["s"], int(c["lane"]) * M.LANE_W, c["h"])
			add_child(mi)
			coin_nodes[id] = mi
		var node: MeshInstance3D = coin_nodes[id]
		node.rotation = Vector3(0, spin + float(c["s"]), PI / 2.0)
		node.visible = not c["taken"]
	for id: int in coin_nodes.keys():
		if not seen.has(id):
			(coin_nodes[id] as Node3D).queue_free()
			coin_nodes.erase(id)


# ---- The runner: seen from behind, legs and arms swinging in step, crouching when you squat ----

func _limb(radius: float, length: float, material: Material) -> Node3D:
	var pivot := Node3D.new()
	var mesh := CapsuleMesh.new()
	mesh.radius = radius
	mesh.height = length + 2.0 * radius
	pivot.add_child(_part(mesh, material, Vector3(0, -length / 2.0, 0)))
	return pivot


func _build_runner() -> void:
	runner_root = Node3D.new()
	rider = Node3D.new()
	upper = Node3D.new()  # everything above the hips, lowered when crouching
	leg_l = _limb(0.06, 0.72, mats["pants"])
	leg_r = _limb(0.06, 0.72, mats["pants"])
	leg_l.position = Vector3(-0.1, 0.95, 0)
	leg_r.position = Vector3(0.1, 0.95, 0)
	var hips := CapsuleMesh.new()
	hips.radius = 0.15
	hips.height = 0.4
	upper.add_child(_part(hips, mats["pants"], Vector3(0, 0.98, 0)))
	var torso := CapsuleMesh.new()
	torso.radius = 0.16
	torso.height = 0.68
	upper.add_child(_part(torso, mats["shirt"], Vector3(0, 1.32, 0)))
	var head := SphereMesh.new()
	head.radius = 0.12
	head.height = 0.24
	upper.add_child(_part(head, mats["skin"], Vector3(0, 1.72, 0)))
	var cap := SphereMesh.new()
	cap.radius = 0.135
	cap.height = 0.135
	cap.is_hemisphere = true
	upper.add_child(_part(cap, mats["cap"], Vector3(0, 1.74, 0)))
	arm_l = _limb(0.045, 0.5, mats["shirt"])
	arm_r = _limb(0.045, 0.5, mats["shirt"])
	arm_l.position = Vector3(-0.23, 1.5, 0)
	arm_r.position = Vector3(0.23, 1.5, 0)
	upper.add_child(arm_l)
	upper.add_child(arm_r)
	rider.add_child(leg_l)
	rider.add_child(leg_r)
	rider.add_child(upper)
	runner_root.add_child(rider)
	add_child(runner_root)


func _update_runner(dt: float, crashing: bool) -> void:
	var p := M.at(game.s, game.x, game.h)
	runner_root.position = p
	# Follows the road, and leans into a lane change
	runner_root.rotation = Vector3(0, -atan(M.road_dx(game.s)), (game.lane * M.LANE_W - game.x) * -0.15)
	crouch += ((1.0 if game.ducking and not game.air else 0.0) - crouch) * minf(1.0, dt * 14.0)
	if crashing:
		var f := 1.0 - (game.crash_until - game.clock) / 1200.0
		rider.rotation = Vector3(-f * 2.2, 0, 0)
		rider.position = Vector3(0, -0.3 * f, -f * 1.2)
	else:
		rider.rotation = Vector3.ZERO
		rider.position = Vector3.ZERO
		var drop := crouch * (M.STAND_H - M.DUCK_H)
		upper.position.y = -drop
		leg_l.scale.y = 1.0 - drop / 0.95
		leg_r.scale.y = leg_l.scale.y
	# Legs swing in step with you while you run, and at an easy jog otherwise. In the air they
	# hold a hurdler's split.
	var moving := game.phase == "play" and not game.paused and not crashing
	var swing := 0.0
	if moving and game.air:
		swing = 1.2
	elif moving:
		stride_phase += dt * TAU * (game.run_spm / 120.0 if game.run_streak > 1 else 1.2)
		swing = sin(stride_phase)
	leg_l.rotation.x = swing * 0.75
	leg_r.rotation.x = -swing * 0.75
	arm_l.rotation.x = -swing * 0.7
	arm_r.rotation.x = swing * 0.7
	# Blinks while you can't be hit, right after a crash
	runner_root.visible = not (game.clock < game.safe_until and game.phase == "play" and int(Time.get_ticks_msec() / 120.0) % 2 == 0)

	# Chase camera: behind and above, lower when you duck so the gap reads clearly
	var target := M.at(game.s - 6.0, game.x * 0.5, 2.5 - crouch * 0.4 + game.h * 0.4)
	if not cam_set or cam_pos.distance_to(target) > 20.0:
		cam_pos = target
		cam_set = true
	else:
		cam_pos = cam_pos.lerp(target, 1.0 - exp(-dt * 6.0))
	cam.look_at_from_position(cam_pos, M.at(game.s + 9.0, game.x * 0.5, 1.0))


# ---- Menus and HUD ----

func _label(text: String, size := 16, color := Color.WHITE) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", color)
	return l


func _dialog(width: float) -> Array:
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var panel := PanelContainer.new()
	panel.custom_minimum_size.x = width
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.1, 0.1, 0.1, 0.92)
	style.set_corner_radius_all(12)
	panel.add_theme_stylebox_override("panel", style)
	var margin := MarginContainer.new()
	for side in ["left", "right", "top", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 20)
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 10)
	margin.add_child(box)
	panel.add_child(margin)
	center.add_child(panel)
	return [center, box]


func _banner_label(top: float, size: int) -> Label:
	var l := _label("", size)
	l.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	l.offset_top = top
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.6))
	l.add_theme_constant_override("outline_size", 10)
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return l


func _build_ui() -> void:
	var ui := CanvasLayer.new()
	add_child(ui)

	status_label = _label("", 15)
	status_label.position = Vector2(16, 12)
	status_label.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.6))
	status_label.add_theme_constant_override("outline_size", 6)
	ui.add_child(status_label)

	hud = _label("", 20)
	hud.set_anchors_and_offsets_preset(Control.PRESET_TOP_RIGHT)
	hud.grow_horizontal = Control.GROW_DIRECTION_BEGIN
	hud.offset_right = -16
	hud.offset_top = 12
	hud.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	hud.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.6))
	hud.add_theme_constant_override("outline_size", 8)
	ui.add_child(hud)

	meters = Meters.new()
	meters.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_LEFT)
	meters.offset_left = 16
	meters.offset_top = -270
	meters.mouse_filter = Control.MOUSE_FILTER_IGNORE
	ui.add_child(meters)

	banner_label = _banner_label(140, 64)
	banner_small = _banner_label(225, 24)
	pop_label = _banner_label(320, 26)
	ui.add_child(banner_label)
	ui.add_child(banner_small)
	ui.add_child(pop_label)

	bar = HBoxContainer.new()
	bar.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM)
	bar.grow_horizontal = Control.GROW_DIRECTION_BOTH
	bar.grow_vertical = Control.GROW_DIRECTION_BEGIN
	bar.offset_bottom = -14
	bar.add_theme_constant_override("separation", 10)
	bar.add_child(_label("Space: pause", 14, Color(1, 1, 1, 0.8)))
	pause_button = Button.new()
	pause_button.text = "Pause"
	pause_button.pressed.connect(game.toggle_pause)
	bar.add_child(pause_button)
	var menu_button := Button.new()
	menu_button.text = "Menu"
	menu_button.pressed.connect(_show_menu)
	bar.add_child(menu_button)
	ui.add_child(bar)

	# The menu
	var made := _dialog(560)
	menu = made[0]
	var box: VBoxContainer = made[1]
	box.add_child(_label("Ready to run", 24))
	var how := _label(
		"1. Put the stick in a snug front trouser pocket and stand facing the screen, with some clear floor around you.\n"
		+ "2. When the run starts, stand still for 2 seconds, then do one squat.\n"
		+ "3. Orange obstacles are low: jump. Blue ones are high: squat until you're under.\n"
		+ "4. Red barriers block part of the road: take a quick step left or right to move one lane over.\n"
		+ "5. Coins lie along the lanes between obstacles: step into their lane to pick them up, and jump for the ones in the air.\n"
		+ "6. Run on the spot, lifting your knees, to go faster: obstacles cleared at speed score more.",
		15, Color(0.8, 0.8, 0.78))
	how.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	box.add_child(how)
	box.add_child(_label("LEVEL", 12, Color(0.6, 0.6, 0.58)))
	var levels := HBoxContainer.new()
	for key: String in M.LEVELS:
		var b := Button.new()
		b.text = M.LEVELS[key]["label"]
		b.toggle_mode = true
		b.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		b.pressed.connect(func() -> void:
			game.level = key
			_render_menu()
			_save_settings())
		levels.add_child(b)
		level_buttons[key] = b
	box.add_child(levels)
	level_desc = _label("", 14, Color(0.8, 0.8, 0.78))
	level_desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	level_desc.custom_minimum_size.y = 44
	box.add_child(level_desc)
	box.add_child(_label("STICK ADDRESS (shown on its screen)", 12, Color(0.6, 0.6, 0.58)))
	host_edit = LineEdit.new()
	host_edit.text = stick.host
	host_edit.text_submitted.connect(func(_t: String) -> void: _apply_host())
	host_edit.focus_exited.connect(_apply_host)
	box.add_child(host_edit)
	var actions := HBoxContainer.new()
	actions.add_theme_constant_override("separation", 12)
	var start := Button.new()
	start.text = "  Start  "
	start.pressed.connect(_start)
	actions.add_child(start)
	actions.add_child(_label("or press Space, or the stick's M5 button", 14, Color(0.8, 0.8, 0.78)))
	box.add_child(actions)
	ui.add_child(menu)

	# The end screen
	made = _dialog(460)
	over = made[0]
	box = made[1]
	box.add_child(_label("Run over", 24))
	over_summary = _label("", 18)
	box.add_child(over_summary)
	actions = HBoxContainer.new()
	actions.add_theme_constant_override("separation", 12)
	var again := Button.new()
	again.text = "  Run again  "
	again.pressed.connect(_start)
	actions.add_child(again)
	var to_menu := Button.new()
	to_menu.text = "Menu"
	to_menu.pressed.connect(_show_menu)
	actions.add_child(to_menu)
	box.add_child(actions)
	over.visible = false
	ui.add_child(over)


func _apply_host() -> void:
	var host := host_edit.text.strip_edges()
	if host == "" or host == stick.host:
		return
	stick.connect_to(host)
	_save_settings()


func _render_menu() -> void:
	for key: String in level_buttons:
		(level_buttons[key] as Button).button_pressed = key == game.level
	level_desc.text = M.LEVELS[game.level]["desc"]


func _start() -> void:
	_apply_host()
	host_edit.release_focus()
	game.start_game()


func _show_menu() -> void:
	game.show_menu()
	_hide_banner()
	over.visible = false


# Space / M5 button: start from the menu or the end screen, pause during a run
func _on_start_key() -> void:
	if game.phase == "menu" or game.phase == "over":
		_start()
	else:
		game.toggle_pause()


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_SPACE:
			_on_start_key()
			get_viewport().set_input_as_handled()
		elif event.keycode == KEY_ESCAPE and game.phase != "menu":
			_show_menu()


func _banner(text: String, color: Color, small := "", ms := 900.0) -> void:
	banner_label.text = text
	banner_label.add_theme_color_override("font_color", color)
	banner_small.text = small
	banner_until = Stick.now_ms() + ms


func _hide_banner() -> void:
	banner_until = 0.0


func _pop(text: String) -> void:
	pop_label.text = text
	pop_until = Stick.now_ms() + 900.0


func _drain_events() -> void:
	while not game.events.is_empty():
		var e: Dictionary = game.events.pop_front()
		match e["type"]:
			"start":
				over.visible = false
				_banner("Stand still", Color.WHITE, "learning how the stick sits…", 2000.0)
			"squat_once":
				_banner("Squat once", Color.WHITE, "down, then stand up again", 1e9)
			"go":
				_banner("Go!", Color.WHITE)
			"paused":
				_banner("Paused", Color.WHITE, "", 1e9)
			"resumed":
				if game.phase == "calibrate" and game.has_up0:
					_banner("Squat once", Color.WHITE, "down, then stand up again", 1e9)  # still waiting
				else:
					_hide_banner()
			"real_jump":
				game.best_jump = maxi(game.best_jump, e["cm"])
				_pop("%d cm" % e["cm"])
			"cue":
				var kind: String = e["kind"]
				var dir: int = e["dir"]
				if kind == "low":
					_banner("JUMP!", Color("ffb066"), "", 700.0)
				elif kind == "high":
					_banner("DUCK!", Color("8cc4ff"), "", 700.0)
				else:
					_banner("LEFT!" if dir < 0 else ("RIGHT!" if dir > 0 else "STEP ASIDE!"), Color("ff9dbb"), "", 700.0)
			"crash":
				if game.phase == "play":
					var kind: String = e["kind"]
					var hint := "jump over the orange ones" if kind == "low" else ("squat under the blue ones" if kind == "high" else "step aside from the red ones")
					_banner("Ouch!", Color("ffb3a8"), hint, 1400.0)
			"recal":
				_pop("Stick moved: re-centred")
			"over":
				_save_settings()
				over_summary.text = (
					"Score %d%s\nDistance %d m   ·   Coins %d   ·   Cleared %d\nJumps %d   ·   Squats %d   ·   Best jump %d cm\nTop speed %d km/h   ·   Steps %d\n\n%s. Best: %d."
					% [e["score"], "   — new best!" if e["is_best"] else "", floori(game.s), game.coins, game.cleared, game.jumps, game.squats,
						game.best_jump, roundi(game.top_v * 3.6), game.run_steps, M.LEVELS[game.level]["label"], int(game.best.get(game.level, 0))])
				get_tree().create_timer(1.2).timeout.connect(func() -> void:
					if game.phase == "over":
						over.visible = true)


func _update_ui() -> void:
	var now := Stick.now_ms()
	status_label.text = "M5Stick Runner  ·  " + stick.status
	status_label.add_theme_color_override("font_color", Color("9be29b") if stick.connected else Color("ffb3a8"))
	menu.visible = game.phase == "menu"
	bar.visible = game.phase != "menu"
	pause_button.visible = game.phase == "play" or game.phase == "calibrate"
	pause_button.text = "Resume" if game.paused else "Pause"
	var in_run := game.phase != "menu"
	hud.visible = in_run
	meters.visible = in_run
	if in_run:
		hud.text = "%s%s    Score %d    %d km/h    Coins %d    Cleared %d    Jumps %d    Squats %d" % [
			"♥".repeat(game.lives), "♡".repeat(maxi(0, game.lv_lives - game.lives)), game.run_score(),
			roundi((game.v if game.phase == "play" else 0.0) * 3.6), game.coins, game.cleared, game.jumps, game.squats]
		meters.angle = game.angle
		meters.duck = game.lv_duck
		meters.ducking = game.ducking
		meters.side_roll = game.side_roll if game.phase == "play" else 0.0
		meters.lane = game.lane
		meters.queue_redraw()
	banner_label.visible = now < banner_until
	banner_small.visible = banner_label.visible
	pop_label.visible = now < pop_until


func _process(delta: float) -> void:
	stick.poll()
	_drain_events()
	if stick.ready():
		game.simulate(stick.device_now())
		_drain_events()
	_update_chunks()
	_update_obstacles()
	_update_coins()
	var crashing := game.clock < game.crash_until and game.phase != "menu"
	_update_runner(minf(delta, 0.1), crashing)
	_update_ui()
	if probe and game.phase == "calibrate" and stick.ready():
		game.up0 = Stick.up_body(stick.q)
		game.has_up0 = true
		game.run_fwd = game.up0.cross(Vector3.RIGHT).normalized()
		game.has_fwd = true
	if probe and Stick.now_ms() - _probe_at > 1000.0:
		_probe_at = Stick.now_ms()
		print("probe: %s  samples/s=%d  phase=%s  angle=%.1f  up=%s  w=%.1f  fwd=%s  side_v=%.2f  spm=%.0f  s=%.1f" % [
			stick.status, _probe_samples, game.phase, game.angle, Stick.up_body(stick.q), stick.w.length(), game.has_fwd, game.side_v, game.run_spm, game.s])
		_probe_samples = 0
