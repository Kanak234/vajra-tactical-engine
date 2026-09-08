# VAJRA — Controls & How To Play

## Mission select

The game opens on the operation file. `UP`/`DOWN` or number keys pick a
mission, `ENTER` deploys. Missions unlock in order; your best time and ghost
status are saved to `vajra_campaign.json` and persist between sessions.

| Mission | Setting | What makes it different |
|---|---|---|
| 1. THE QUIET STATION | Daylight compound | The tutorial. Open ground, room to retreat |
| 2. UPLINK | Night ridge corridor | Darkness cuts guard vision to ~34m, but the corridor is narrow — less room to break contact. One drone on overwatch |
| 3. COLD STORAGE | Indoor robotics lab | Almost all enemies are machines: 360° vision, short range, **deaf, and not on the radio net**. Two human technicians are the only ones who can call for help |


## Controls

| Input | Action |
|---|---|
| `W A S D` | Move |
| `Mouse` | Look |
| `Left Shift` | Sprint (loud — guards hear you) |
| `Left Ctrl` / `C` | Crouch (near-silent, tighter aim) |
| `Space` | Jump |
| `Left Mouse` | Fire |
| `Right Mouse` | Aim down sights (zoom, far tighter spread) |
| `R` | Reload |
| `1` / `2` | Silenced rifle / sidearm |
| `E` | Interact — sabotage an objective |
| `H` | Use medkit (you start with 2) |
| `F3` | Debug overlay — per-pass timings, draw counts, particles, threads |
| `F7` | Toggle SSAO |
| `F8` | Toggle bloom |
| `F10` | Toggle FXAA |
| `F5` | Hot-reload shaders |
| `F6` / `F9` | Save / load |
| `Esc` | Pause & release mouse |
| `Enter` | Deploy / next mission / retry |
| `M` | Back to mission list (from pause or an end screen) |

## The mission: THE QUIET STATION

1. **Reach the south breach** — the gap in the perimeter wall
2. **Sabotage the comms relay** — west building, door faces east. Stand next to it, press `E`
3. **Wipe the data core** — inside the hangar to the north. Press `E`
4. **Extract** — the green pad lights up at your insertion point once both are done

## How to actually win

**Read the awareness meter.** The bar in the centre of the screen is the game.
Amber means someone is suspicious. Red means you are made.

**Awareness decays.** Break line of sight and wait. This is the single most
important mechanic — retreating genuinely works, unlike most shooters.

**Crouch almost always.** Crouching cuts your noise to a fraction and tightens
your aim. Sprinting is for when you have already been seen.

**The sidearm is a trap.** It is unsuppressed. One shot alerts the entire
sector. It exists as a panic button, not a weapon.

**Guards are colour-coded while you develop:**
grey = idle, yellow = suspicious, orange = searching, red = in combat.

**Kill the relay guard first.** Squad members share contacts over radio, so
anyone who spots you tells everyone within 120 metres.

## Finishing undetected

Complete the mission without any guard reaching full alert and the end screen
shows **UNDETECTED — GHOST**. That is the real win condition. Anyone can shoot
their way through ten guards; the game is about not having to.
