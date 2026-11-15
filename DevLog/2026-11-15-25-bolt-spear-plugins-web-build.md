# The Bolt through houses, the spear through the army, fewer plugins, a web build and its zip (2026-11-15 02:45 PST to 03:15)

## When

2026-11-15 02:45 PST to 03:15.

## Asked

The director wrote: "Have the spear sound follow the actual spear rather than staying in one location." "For the
wizard's bolt, do not allow houses or lamp posts to destroy the bolt. Allow it to pass through." "Also allow the
commanders spear to hit all targets in it's path as it goes. I do not believe it currently does". "Also explore what
plugins you can remove to reduce size. Only remove the plugins that would for sure not have any bearing in the
project's shipped build." "After that's all done, do the full testing and make a build for itch to be uploaded".
"include all the relevant md files for the game jam in the zip file as well". "Now list what's left".

## What the director did

Changed the throw and gate sound assets in the editor (committed first as a snapshot).

## What the agent did

Claude Code (Claude Fable 5.2).

- **The Bolt** passes through houses and lamp posts. They still block walking. Trees of a code-made layout, bridge
  rails, the shrine and other blockers still stop it.
- **The commander's spear** hits each of the army in its path once (20, a new setting `CommanderSpearDamage`) and
  flies on. Before, it passed through them without hurting them. It still stops at the wizard.
- **The spear's sound** is moved with the spear every frame.
- **Plugins.** 225 engine plugins were built into the web program. Listed which have code in it (108) and what the
  game's content uses. Left 26 out of the web platform only (the editor is unchanged); 15 more fell away with them
  because nothing else needed them. Kept everything any asset uses (Control Rig and its family, ACL, the Megascans
  Bridge and what it needs), Niagara (effects are planned), and every audio plugin (the agent cannot hear, so could
  not prove them unused).
- **The zip tool** can add the jam documents (`--docs`): `docs/WhatWasUsed.md`, `docs/GDD.md` and the dev logs,
  without their pictures (75 MB), in a `JamDocs` folder.

## Tests

- New unit tests (2, `Tests/BoltPassSpearPierceTests.cpp`), written before the code; they do not compile against
  the old code.
- **A test was changed.** `WebTesting.Commander.HisSpearPassesTheArmyAndHitsTheWizard` said "the skeleton was not
  hurt by it". That is the opposite of the new direction, so it now says the skeleton in its way was hit.
- Scenario `day_nineteen_pickup_gate_throw` also checks that the spear's sound is started and moved.
- **Full suite**: unit tests 228 of 228 pass. Scenarios 33 of 34 pass. `day_two_walls` fails as it has since dev
  log 19: it walks into the barn the director moved. Not from this round.

## Build

Web package (Shipping, Wasm): succeeded.

| | Before (dev log 22) | Now |
|---|---|---|
| Program file (.wasm) | 133 MB | 114.8 MB |
| Program file, compressed | 26 MB | 22.3 MB |
| Content file (.ucas) | 87 MB | 78.2 MB |
| Upload, unzipped | 238 MB | 207.5 MB (with the documents) |
| Plugins built in | 225 | 184 |

Zip: `Saved/JamZips/Screeps_TheBoneKing_Web.zip`, 120.8 MB, `index.html` at the top, under itch.io's limits.

Browser (the app's own browser pane, localhost): the menu loads, the game starts, 30 skeletons fight 40 soldiers,
bones drop, no errors in the console.

## Not done, not checked

- **Frame rate in the browser was not measured.** The browser pane was hidden, where pages are slowed to about one
  frame a second, so the 5 frames a second shown says nothing about the build. The build before this one ran at 147
  to 242.
- **The fog looks stronger in the browser** than in the editor: a clear blue haze. For the director to judge.
- **No sound was heard by the agent.**
- Not played to the gate in the browser. Not tried in Brave.
- Nothing was uploaded.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview.
