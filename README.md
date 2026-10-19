# M5Stick Tennis

Turn an M5StickC into a motion controller. The stick streams its gyro and accelerometer
over WiFi and serves web pages you open in any browser on the same network:

- **Tennis** (`/tennis`): a 3D tennis game where the stick is your racket grip. Play a
  match against a computer opponent or practise against a ball machine.
- **Saber** (`/saber`): a first-person lightsaber game where the stick is the hilt. Block
  the bolts a training droid fires at you and send them back to destroy it.
- **Skate** (`/skate`): an endless downhill skateboard run where the stick is the board.
  Tilt to carve around obstacles and flick the nose up to jump.
- **Runner** (`/runner`): a whole-body game with the stick in your trouser pocket. Run on
  the spot to speed your runner up, really jump over low obstacles and squat under high
  ones, and step sideways to change lane.
- **Red light, green light** (`/redlight`): the first game from Squid Game, with the stick in
  your trouser pocket. Run on the spot while the doll sings, and freeze when she turns around.
- **Motion** (`/`): live gyro/accelerometer charts, a 3D model of the stick that follows
  its orientation, and an air mouse you steer by turning the stick.

## What you need

- An **original M5StickC** (ESP32-PICO, 80×160 screen). It uses M5Unified, so other
  M5Stick models may work but are untested.
- A USB-C cable.
- [PlatformIO](https://platformio.org/install), either the CLI (`pip install platformio`)
  or the VS Code extension.
- A 2.4 GHz WiFi network. The computer or phone you play on must be on the same network
  and have internet access, because the game pages load three.js from a CDN.

## Setup

1. **Clone the repo**

   ```sh
   git clone https://github.com/thesverre/m5stick-tennis.git
   cd m5stick-tennis
   ```

2. **Add your WiFi details.** Copy the example and fill in your network name and
   password. `include/secrets.h` is git-ignored.

   ```sh
   cp include/secrets.h.example include/secrets.h
   ```

3. **Set the serial port.** `platformio.ini` uploads to `/dev/ttyUSB0`. Change
   `upload_port` to match your machine (for example `COM3` on Windows or
   `/dev/cu.usbserial-*` on macOS), or delete the line to let PlatformIO find it.
   On Linux, add yourself to the `dialout` group if you get a permission error.

4. **Build and flash.** Libraries are installed automatically on the first build.

   ```sh
   pio run -t upload
   ```

   The upload runs at 115200 baud because some M5StickC USB bridges fail at higher
   speeds. It takes about a minute.

5. **Open the pages.** When the stick connects, its screen shows `m5stick.local` and its
   IP address. Browse to **http://m5stick.local/** and pick a game from the links at the top. If `.local` names don't resolve
   on your device (some Android phones and older Windows setups), use the IP shown on the
   screen instead.

If the screen says **WiFi failed**, check `secrets.h` and press the M5 (front) button to
retry.

## Playing tennis

1. Hold the stick in your fist like a racket grip, top end toward the racket head, screen
   facing the same way as your palm. The screen stands in for the strings.
2. Point the stick at your screen and press the **side button** to line up the racket.
   Press it again whenever the racket's direction drifts.
3. Pick a level, a mode and your racket hand (right or left), then press **Start** or the
   **M5 button**. The hand setting is shared with the saber game.
4. You stay in place and every ball comes to your forehand or backhand. Swing to meet it
   as it reaches the white ring.

| Level  | What counts |
|--------|-------------|
| Easy   | Timing only. Swing as the shrinking ring meets the white one. Early goes cross-court, late goes down the line. |
| Medium | The racket has to meet the ball, and the face tilt steers the shot. Big sweet spot, with some help aiming and clearing the net. |
| Hard   | Real-size strings, fast balls, no net help. Open the face a few degrees to clear the net, and time the swing to aim. |

**Match** mode uses real tennis scoring (15-30-40, deuce, advantage). The opponent serves
every point, and the first to 3 games wins. **Practice** mode counts how many balls you
get in and your best streak.

| Button | During play |
|--------|-------------|
| Side button | Recenter the racket |
| M5 button | Pause / resume (starts a game from the menu) |

## Playing the saber game

1. Hold the stick in your fist like a saber hilt, top end toward the blade.
2. Point it at your screen and press the **side button** to line up the blade.
3. Pick a level, a mode and your saber hand, then press **Start** or the **M5 button**. Click
   Start at least once if you want sound, because browsers only allow audio after a click.
4. The droid glows red and marks a red ring where its next bolt will come through. Hold the
   blade across that ring before the bolt arrives. A blade held square to the bolt sends it
   straight back at the droid; a slanted one sends it off to the side.

| Level  | What to expect |
|--------|----------------|
| Easy   | Long warnings, slow bolts, a wide blade. Every block flies back into the droid. |
| Medium | Shorter warnings, faster bolts, sometimes two at once. Blocks within 35° of square hit the droid. |
| Hard   | Quick pairs of fast bolts and a real-size blade. Only blocks within 15° of square hit the droid. |

**Waves** mode is a line of droids, each with more hit points and faster than the last. You
have 5 lives, and your best score per level is saved in the browser. **Free swing** has no
droid: just swing the saber and listen to it hum.

The buttons work as in tennis: the side button recenters and the M5 button pauses.

## Playing the skate game

1. Hold the stick flat like a fingerboard: screen up, top end pointing at your screen.
2. Pick a level and press **Start** or the **M5 button**. Press the **side button** (or
   **Set level**) while holding the stick level, so that counts as riding straight.
3. Tilt left or right to lean and carve. Tilt the nose down to tuck and go faster, nose up
   to brake. The road bends, so you have to lean into the curves.
4. Flick the nose up quickly to ollie over logs, cones and rocks; the M5 button also jumps.
   Steer around barriers and cars, ride over ramps for big air, and grab coins.

| Level  | What to expect |
|--------|----------------|
| Easy   | Cruising speed, about 2 s between obstacles, 3 lives. Landings always stick. |
| Medium | Faster, busier, cars rolling down the hill with you. 2 lives; land tilted and you wobble. |
| Hard   | Up to ~85 km/h, about 1 s between obstacles. One hit ends the run, and so does a tilted landing. |

Score is distance in metres plus 10 per coin and 25 per ramp jump. The run speeds up the
further you get, and your best score per level is saved in the browser. Tilt comes from
gravity, so this game never drifts and needs no recentering.

## Playing the runner game

1. Put the stick in a snug front trouser pocket and stand facing the screen with some clear
   floor around you.
2. Pick a level and press **Start** or **Space**. Stand still for 2 seconds while the stick
   learns how it sits in your pocket, then do one squat when asked, so it learns
   which way you face.
3. **Orange** obstacles are low: **jump**. A small hop of a few centimetres is enough.
   **Blue** ones are high: **squat** until you're under them, and stay down for long ones.
4. **Red** barriers block one or two of the road's three lanes: **take a step** to the left
   or right to move one lane over, and step back to return. The sidestep indicator next to
   the squat gauge shows how far your thigh tilts sideways and which lane you're in.
5. **Coins** lie along the lanes between obstacles, in lines of up to six: step into their
   lane to pick them up. Some hang in the air in threes: jump for those. Each is 10 points.
6. **Run on the spot**, lifting your knees, to go faster. Standing still, your runner jogs at
   18 km/h; it reaches 36 km/h at about 240 steps a minute, and eases back down slowly when
   you stop, so you keep most of your speed through a jump or a squat.
7. The squat gauge on the left shows how far your thigh is tilted, with a mark where it
   counts. Each jump's real height pops up on screen.

| Level  | What to expect |
|--------|----------------|
| Easy   | 4.6–6 s between obstacles, a 22° squat is enough, a few one-lane barriers, JUMP!/DUCK!/LEFT!/RIGHT! cues, 3 lives. |
| Medium | 3.4–4.4 s apart, some in pairs, 28° squats, longer holds, some barriers two lanes wide. 2 lives. |
| Hard   | 2.5–3.3 s apart, many pairs, 33° squats, tunnels up to 1.5 s long, many wide barriers, no cues. 1 life. |

Those times are at 25 km/h; the faster you run, the quicker the obstacles come. Score is
distance in metres, plus 10 per coin, plus a bonus per obstacle cleared: 20 at 25 km/h, more
the faster you're going. At the end you get your score, distance, coins, obstacles cleared, jumps, squats, best jump
height, top speed and steps.

How it detects you: a jump is a push-off above 1.3 g followed by the free fall of being in the
air (under 0.5 g), and its height comes from the time in the air. A squat is your thigh tilting
forward from where it was when you stood still. If the stick shifts in your pocket, the game
re-centres itself while you stand still.

Running is the rhythm of that thigh swinging forward: each swing past 12° is one stride (two
steps), and three strides in rhythm set the speed. While you're running, a squat only counts
once your thigh has stayed up longer than a knee lift would (half a stride), and a jump needs
0.1 s of free fall rather than 0.03 s, so that running steps aren't taken for either. The first
knee lift of a run can still show as a brief crouch or hop.

A sidestep tilts the pocket thigh sideways, whichever way you step: 14–21° in a recording of
real steps, against 11° at most while running. So a step is that tilt passing 12.5° while the
thigh isn't far forward. Which way you stepped is the sign of your sideways speed just before
(the sideways acceleration, with gravity taken out, added up over about half a second). Which
way is forward, and so which way is left, comes from the squat at the start, so it holds
however the stick sits in your pocket and whichever way you turn.

`tools/record_motion.py` records the stick's stream while it prompts you through a routine of
sidesteps, running, jumps and a squat; that's what these numbers were tuned on.

## Playing red light, green light

1. Put the stick in a snug front trouser pocket and stand facing the screen.
2. Pick a level and press **Start**, **Space** or the **M5 button**. Stand still for 2 seconds
   while the stick learns how it sits, then wait out the 3-2-1.
3. **Green light**: the doll turns her back and sings. **Run on the spot**, lifting your knees:
   you run at the pace of your strides (1.4 m per stride of the pocket leg), and slow to a stop
   when you stop. The shorter her song, the sooner she turns, and her songs get shorter the
   closer you get to her.
4. **Red light**: she turns to face you. **Freeze.** After a moment to come to a stop, any
   movement she sees costs a life and sends you back 5 m. The movement meter on the left shows
   how much you're moving, with a mark where she sees it.
5. Cross the red finish line, 50 m away, before the time runs out. Nine other players run with
   you, and some of them get caught.
6. While she sings you hear her chant, "mugunghwa kkochi pieotseumnida" (the hibiscus flower has
   bloomed, the call from the Korean children's game this is based on) if your browser has a
   Korean voice, over an original march. The music cuts out when she turns, and a heartbeat
   plays while she watches. **M** or the Music button turns the music off.
7. Each crossing starts a new round, harder than the last: songs 15% shorter, stares 0.3 s
   longer, 12% less time to stop, a 10% sharper eye and 4 s less on the clock. Your lives carry
   over. The game ends when you run out of lives or time; your score is the rounds you cleared.

The level sets where round 1 starts:

| Level  | Round 1 |
|--------|---------|
| Easy   | 80 s, songs of 3–5.5 s, 0.8 s to stop, only clear movement is seen. 3 lives. |
| Medium | 65 s, songs of 2–4.5 s, 0.6 s to stop, smaller movements are seen. 2 lives. |
| Hard   | 60 s, songs of 1.5–4 s, 0.45 s to stop, she sees you sway. 1 life. |

Movement is the stick's rotation rate and how far its acceleration is from 1 g, smoothed over
80 ms; it has to last 80 ms to be seen. In a recording, standing still read up to 10°/s and
standing just after running up to 31°/s, against 100–135°/s while running or stepping, so Easy
sees 70°/s, Medium 50°/s and Hard 38°/s (the `LEVELS` table in `web/redlight.html`).

## The runner as a Godot game

`godot-runner/` is the same runner game as a desktop app built with the
[Godot engine](https://godotengine.org/) (4.7 or later). The stick needs the firmware from this
repo and has to be on the same network; the game reads its motion stream from
`ws://m5stick.local/ws`.

```sh
godot --path godot-runner      # play
godot --path godot-runner -e   # open it in the Godot editor
```

It plays by the same rules, with the same detection and levels. If `m5stick.local` doesn't
resolve, type the IP shown on the stick's screen into the menu's address field. Space or the
stick's M5 button starts and pauses, Esc goes back to the menu. Level, address and best scores
are remembered. It has no sound yet.

| File | Contents |
|------|----------|
| `scripts/stick.gd` | The stick's stream and the orientation filter (`web/motion.js` and `web/controller.js`) |
| `scripts/model.gd` | Rules and motion detection, with the tuning constants and `LEVELS` at the top |
| `scripts/main.gd` | The 3D scene, menus and HUD, all built in code |

If you tune a threshold in `web/runner.html`, change it in `scripts/model.gd` too.

## Air mouse

On the main page, hold the stick like a remote: screen up, top end pointing at your
screen. Turn it left/right and up/down to move the pointer. The M5 button clicks (hold it
to draw), and the side button re-centers the pointer.

## Tuning

- **Tennis difficulty:** the `LEVELS` table near the top of the script in `web/tennis.html`
  sets each level's timing window, hitting area, aim and net help, and the opponent's
  speed, error rate and reactions.
- **Saber difficulty:** the `LEVELS` table in `web/saber.html` sets each level's warning time,
  bolt speed, bursts, blade reach and the angle a block needs to hit the droid.
- **Runner difficulty:** the `LEVELS` table in `web/runner.html` sets time between obstacles,
  how long high ones are, jump length, squat angle, pairs, cues and lives; the detection
  thresholds (`PUSH_G`, `AIR_G`, `LAND_G` for jumps, `STRIDE_HI`, `DUCK_DWELL`,
  `AIR_SAMPLES_RUN` for running, `SIDE_TILT`, `SIDE_V_MIN`, `SIDE_LOCK_MS` for sidesteps) are just
  above it; `side` and `wide` in the table set how many barriers there are, and `JOG`, `TOP`, `SPM_TOP` and `COAST_S`
  set how cadence becomes speed.
- **Skate difficulty:** the `LEVELS` table in `web/skate.html` sets speeds, time between
  obstacles, lives and landings; `STEER`, `LEAN_FULL` and `OLLIE_DPS` set how the stick
  steers and how hard a flick has to be to jump.
- **Air mouse:** `JITTER_DPS` (dead zone for hand shake) and `CLICK_FREEZE_MS` (pointer
  freeze around clicks) in `web/index.html`.

After editing anything in `web/`, rebuild and upload with `pio run -t upload`; the pages are
compressed into the firmware automatically as part of the build.

## How it works

The stick samples its IMU (MPU6886) at 100 Hz and streams batches of samples over a
WebSocket at `/ws`. The pages do all the processing in the browser:

- A Mahony filter fuses gyro and accelerometer into an orientation. Tilt is corrected by
  gravity; heading comes from the gyro and slowly drifts, which is why there is a
  recenter button. The gyro's offset is learned whenever the stick is still.
- The stick can't measure its position, so the racket's and saber's positions come from an
  arm model: the hands move with the direction the stick points.
- The games' physics runs in 2 ms steps against the stick's interpolated motion, so fast
  swings don't pass through the ball or the bolts.
- The pages are gzipped at build time and stored in flash that way (about 210 KB of pages
  take about 70 KB); the stick sends them compressed and the browser unpacks them.

## Project layout

| File | Contents |
|------|----------|
| `src/main.cpp` | WiFi, IMU sampling, WebSocket stream, web server |
| `web/index.html` | Motion page (`/`): charts, 3D stick, air mouse |
| `web/tennis.html` | Tennis game (`/tennis`) |
| `web/saber.html` | Saber game (`/saber`) |
| `web/skate.html` | Skate game (`/skate`) |
| `web/runner.html` | Runner game (`/runner`) |
| `web/redlight.html` | Red light, green light (`/redlight`) |
| `web/motion.js` | Shared `/motion.js`: stream client and orientation filter |
| `web/controller.js` | Shared `/controller.js` for the games: pose history, timing, recenter, buttons, swing speed |
| `godot-runner/` | The runner game as a Godot project |
| `tools/record_motion.py` | Records the stick's motion while prompting you through a routine, for tuning the runner |
| `tools/embed_web.py` | Build step: gzips `web/` into `include/web_assets.h` (generated, not committed) |
| `include/secrets.h.example` | Template for your WiFi details |
