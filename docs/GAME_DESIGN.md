# VAJRA — Game Design Document
**Version 0.1 · Tactical sci-fi infiltration · Linux · Custom C++ engine**

---

## 1. One-line pitch

A lone Indian special-operations officer infiltrates a rogue techno-industrial
cartel that has seized an abandoned high-altitude research corridor — using
patience, silence, and a single sniper rifle instead of firepower.

---

## 2. Setting

**Fictional. No real nation is depicted as an enemy.**

The world is India, roughly fifteen years from now. The country runs the
**Himalayan Deep Science Corridor** — a chain of remote research stations
built for anti-gravity, high-altitude propulsion, and autonomous robotics work.
The corridor was mothballed after a funding collapse.

Something has restarted it.

A stateless arms syndicate calling itself the **Vritra Combine** — named after
the mythological serpent that hoards the world's water — has quietly occupied
the abandoned stations. They are not a country. They are ex-contractors,
disgraced researchers, and mercenaries with no flag, selling weaponised
anti-gravity tech to whoever pays. Their soldiers wear unmarked grey.

This choice is deliberate and it is also the *better* design: a faceless,
flagless enemy lets the player feel isolated and outnumbered without the game
becoming about real people. It is exactly how Project IGI worked.

---

## 3. Protagonist

**Captain Aarav Deshmukh**, callsign **VAJRA**.

- Ex-Para SF, seconded to a fictional intelligence outfit: the **Bureau of
  Strategic Technologies (BST)**.
- Works alone in the field. His only contact is **Meera Iyengar**, his handler,
  a voice in his ear who reads satellite feeds and argues with him about risk.
- He is not a superhero. He carries one primary weapon, one sidearm, limited
  medkits, and no regenerating health.

The Aarav–Meera radio relationship is the emotional spine of the game — the
Anya Oskarsson dynamic from IGI, but with more friction and more warmth.

---

## 4. Core gameplay loop

```
Observe from distance  →  Plan a route  →  Execute silently
        ↑                                        ↓
   Reposition   ←   Break contact   ←   Get spotted (often)
```

**Design pillars**

| Pillar | What it means in code |
|---|---|
| **Patience over reflex** | Slow movement speeds, heavy weapon sway, no sprint-strafing |
| **Observation is a mechanic** | Binoculars + map marking; guards must be studied before acting |
| **Mistakes cascade** | One alerted guard radios the whole sector; alarm state persists |
| **No regenerating health** | Medkits are found, not granted |
| **One life per mission** | Mid-mission saves are limited, not unlimited |

---

## 5. Signature systems

### 5.1 Awareness, not detection

Guards do not flip from blind to omniscient. Each holds an `awareness` float
from 0 to 1, driven by distance, view angle, line of sight, and noise. It
climbs while you are visible and **decays when you break contact** — which is
what makes retreating a real tactic instead of a death sentence.

`Idle → Suspicious → Searching → Combat → Retreating`

Implemented in `src/AI/PerceptionSystem.cpp`.

### 5.2 The Anti-Gravity Harness (Kavach Mk-III)

The one sci-fi toy, introduced in Act II. Not a jetpack — a **fall-arrest and
short-burst vertical assist** rig with a hard energy budget.

- Slows descent to survive a 40m drop
- One short vertical boost to reach a ledge or rooftop
- Recharges only while stationary and unseen

It opens vertical routes without turning the game into an arena shooter, and
the recharge condition rewards the same patience the rest of the game asks for.

### 5.3 Squad radio net

Guards share a squad channel. Kill one silently and his squad notices he stopped
checking in — after a delay. Kill one loudly and the sector alarm fires
immediately. Cutting the comms relay in a sector buys you a window of silence.

---

## 6. Mission structure — Act outline

**Act I — Signal**
1. *Cold Entry* — Night HALO into the lower corridor. Tutorial by doing.
2. *The Quiet Station* — First occupied research post. Learn the awareness system.
3. *Uplink* — Cut a comms relay while a patrol rotates through.

**Act II — Ascent**
4. *Thin Air* — First use of the Kavach harness on a cliff face.
5. *Warehouse Nine* — Discover the Combine is shipping the tech out.
6. *The Convoy* — Vehicle interception on a mountain road.

**Act III — Vritra**
7. *Cold Storage* — The autonomous robotics lab. Machine enemies, no radio net.
8. *Ninety Minutes* — Escort a defecting researcher out on foot, no rifle.
9. *The Serpent's Head* — The core facility. Shut it down, walk out.

The ending is a shutdown and an extraction, not a conquest. Aarav wins by
making the technology unsellable — not by defeating a country.

---

## 7. Art direction

- **Palette:** desaturated slate blues and stone greys, punched with the warm
  amber of Indian station lighting and emergency strips
- **Environments:** high-altitude concrete brutalism, snow-scoured steel,
  prayer flags left by whoever built the place, Devanagari signage weathering off
- **Audio:** wind as the dominant ambient layer; the score is sparse — a lone
  bansuri and low sustained strings, silence during infiltration

---

## 8. What is explicitly out of scope

- Real countries, real militaries, real political figures, real conflicts
- Real-world religious or communal conflict
- Torture or civilian harm as gameplay

These are not just ethical guardrails — they are also what every storefront and
publisher will ask about, and what would get an Indian indie title flagged in
international markets.
