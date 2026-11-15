# Brute slam, melee that stands, swing fix, new prices, assassin model, houses and shrine (2026-11-13 21:53 PST to 22:40 PST)

## When

2026-11-13 21:53 PST to 22:40 PST.

## Asked

The director wrote:

- "Create a new thread I can type to put down what was used in this game jam to create this game."
- "The melee models to a T/A pose when melee. This should be an easy fix for you to finish."
- "Make the enemy brute have a different attack, which is a aoe slam attack in front of it. It starts as a tiny red
  circle and builds up to the aoe size. Upon reaching that size, .3s later, they slam down and do big damage. At this
  stage, ensure all melee stand still (can still be animated/play montage), meaning they also do not turning."
- "Change the cost of summoning to 2 for now. Additionally, have the spear only cost 3 to shoot."
- The enemy assassin and archer models, "do the skeleton fix"; import every .glb in the Houses folder and the shrine's;
  "ensure doors/doorway entrances line up to about 230cm tall so it's to scale with the player".

## What the agent did

Claude Code (Claude Fable 5.2).

- **The credits thread** was started as its own session; it keeps `docs/WhatWasUsed.md`.
- **The bind pose in melee.** Cause: the swing animation was given to the hidden swing figures before they were
  registered, and the engine drops an animation given that early. All 18 swing figures had nothing to play. It is now
  given after registering.
- **Brute slam.** A red circle grows in front of the brute for 1 second, turns bright, and 0.3 seconds later the slam
  does 40 to each of the army inside and 35% of the Overwhelm meter to the wizard if he is inside. The brute stands
  and does not turn from the first of the circle to the slam, then rests 1.2 seconds. Only the 0.3 seconds came from
  the director; the other numbers are the agent's guesses.
- **Melee stands.** Every melee unit stands and keeps its aim from the start of a blow until it lands.
  A problem this caused, found by an old test: nothing that runs (an archer) could ever be hit. So a blow that was
  started stays wound up while the unit chases, and lands when it catches up.
- **Prices.** 2 bones a skeleton, 3 bones a spear.
- **Assassin.** Rig cleaned, imported at 165 units, drawn as a model, growing into view as its cloak drops.
- **Archer: not done.** The file has no rig in it (no armature at all), so there is nothing to fix. It needs to be
  exported again with its skeleton.
- **Houses and shrine.** Each .glb was rendered straight-on, its doorway measured, and the model scaled so the
  doorway is about 230 units; the doorstep sits at height 0 and the slab under the ground. Imported as static meshes
  with materials into `Content/Buildings`. The shrine is a ruin with no doorway: sized by its window sill (chest
  height), the agent's choice. They are imported only: the level still draws boxes.

## Tests

- 5 new unit tests: the slam grows then lands; the brute stands and does not turn; a striker stands and does not
  turn; the two prices; the buildings' scale. The first four failed against stand-ins first. The building test was
  not run before the import. 145 pass.
- The swing fix is checked in the scenario `day_nine_models` (`swingsReady` 18). Run against the old order first:
  it read 0.
- Old tests changed, plainly:
  - Six tests whose numbers were for 3 bones a skeleton now set that price themselves.
  - `day_two_bones`: its bone counts follow the new price.
  - `day_nine_models`: the model count is checked earlier (the brute's slam now kills skeletons before the old
    check), and the exact bone counts after the spear became ranges, because bones picked up in the fight vary.
    That part is a looser check than before; the spear's price is covered by a unit test.
- New scenario `day_ten_slam`. 17 scenarios pass. Its first two versions failed on the agent's own timing (the army
  killed the brute; the hold was too short), not on the game.
- Screenshots looked at: `Images/2026-11-13-10-slam_grows.png`, `-slam_full.png`, `-models-fight.png`.

## Build

Shipping web build, `BUILD SUCCESSFUL`, 225 MB staged, served at http://localhost:8900/. It loads and runs in the
browser pane. No itch.io zip.

## Not done, not checked

- The swing and the slam were not watched in the browser by the agent (the director was playing in the pane). On the
  desktop the swing was not judged in motion either; only that the figures have the animation.
- The brute is still a box; no slam animation.
- Balance is untested by a person: a slam kills plain skeletons outright, and skeletons now stand while they strike.
- The archer model. The houses and shrine are not placed in the level.
- `02_Brute` in the models folder was not asked for and not touched.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; Blender 5.1 (rig cleanup,
building renders and scaling); the Claude desktop app's browser pane. The assassin, houses and shrine meshes and
textures came from the partner mesh service (Tripo), made by the director.
