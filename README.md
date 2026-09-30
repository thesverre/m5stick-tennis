# M5Stick Tennis

Turn an M5StickC into a motion controller. The stick streams its gyro and accelerometer
over WiFi and serves web pages you open in any browser on the same network:

- **Tennis** (`/tennis`): a 3D tennis game where the stick is your racket grip. Play a
  match against a computer opponent or practise against a ball machine.
- **Motion** (`/`): live gyro/accelerometer charts, a 3D model of the stick that follows
  its orientation, and an air mouse you steer by turning the stick.

## What you need

- An **original M5StickC** (ESP32-PICO, 80×160 screen). It uses M5Unified, so other
  M5Stick models may work but are untested.
- A USB-C cable.
- [PlatformIO](https://platformio.org/install), either the CLI (`pip install platformio`)
  or the VS Code extension.
- A 2.4 GHz WiFi network. The computer or phone you play on must be on the same network
  and have internet access, because the tennis page loads three.js from a CDN.

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
   IP address. Browse to **http://m5stick.local/tennis**. If `.local` names don't resolve
   on your device (some Android phones and older Windows setups), use the IP shown on the
   screen instead.

If the screen says **WiFi failed**, check `secrets.h` and press the M5 (front) button to
retry.

## Playing tennis

1. Hold the stick in your fist like a racket grip, top end toward the racket head, screen
   facing the same way as your palm. The screen stands in for the strings.
2. Point the stick at your screen and press the **side button** to line up the racket.
   Press it again whenever the racket's direction drifts.
3. Pick a level and a mode, then press **Start** or the **M5 button**.
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

## Air mouse

On the main page, hold the stick like a remote: screen up, top end pointing at your
screen. Turn it left/right and up/down to move the pointer. The M5 button clicks (hold it
to draw), and the side button re-centers the pointer.

## Tuning

- **Tennis difficulty:** the `LEVELS` table near the top of the script in `src/tennis.h`
  sets each level's timing window, hitting area, aim and net help, and the opponent's
  speed, error rate and reactions.
- **Air mouse:** `JITTER_DPS` (dead zone for hand shake) and `CLICK_FREEZE_MS` (pointer
  freeze around clicks) in `src/page.h`.

## How it works

The stick samples its IMU (MPU6886) at 100 Hz and streams batches of samples over a
WebSocket at `/ws`. The pages do all the processing in the browser:

- A Mahony filter fuses gyro and accelerometer into an orientation. Tilt is corrected by
  gravity; heading comes from the gyro and slowly drifts, which is why there is a
  recenter button. The gyro's offset is learned whenever the stick is still.
- The stick can't measure its position, so the tennis racket's position comes from an
  arm model: the hand sits at the end of an arm that points the way the racket does.
- The game's physics runs in 2 ms steps against the racket's interpolated motion, so fast
  swings don't pass through the ball.

## Project layout

| File | Contents |
|------|----------|
| `src/main.cpp` | WiFi, IMU sampling, WebSocket stream, web server |
| `src/motion.h` | Shared `/motion.js`: stream client and orientation filter |
| `src/page.h` | Motion page (`/`): charts, 3D stick, air mouse |
| `src/tennis.h` | Tennis game (`/tennis`) |
| `include/secrets.h.example` | Template for your WiFi details |
