# Web build of the last four rounds, swords swapped, settings kept in the browser (2026-11-15 01:05 PST to 01:41 PST)

## When

2026-11-15 01:05 PST to 01:41 PST.

## Asked

The director wrote: "steel for skeleton and guard, iron for grunt. Do the First 3 in the must do now" (the web build
and browser check, the download size, playing it in the browser).

## What the agent did

Claude Code (Claude Fable 5.2).

- **Swords.** Skeleton and Guard hold the steel sword, the enemy's soldier the iron longsword.
- **Web build.** Shipping package of the Outskirts level. Everything since the last web build (sounds, music,
  montages, weapons, the sweep, 3D sound, volume) compiled for the web and runs.
- **Size.** 274 MB staged (58 files); 238 MB without the compressed copies, which is what an itch.io zip would hold; largest file the .wasm at 133 MB; the browser downloaded 125 MB. Under itch.io's 500 MB total and 200 MB per file.
- **Played in the built-in browser pane.** Menu, March, walking, Bolt, the see-through circle, 30 skeletons against
  about 40 soldiers (test keys), weapons in hand, deaths, pause, settings, being Overwhelmed and the run restarting.
  147 to 242 frames a second with about 270 figures on screen.
- **Found and fixed: settings were lost on reload.** The page did not tell the engine which project it is, so the
  folder the browser keeps was not the one the game saves to. `PageArgs` in `Config/DefaultEngine.ini` now carries
  `-project=`. Shown first by editing the staged page, then packaged again. In the second package a changed Quality survived a reload.
- **Two fatal errors in the console were not the build's**: they came from the tab left open on the old build while
  the package was being written. Four fresh loads after it showed none.
- **Checked, not a fault**: walking back from the start for about 2 seconds fails the run. It is the battlefield
  edge rule and the editor does the same.

## Tests

- **Test changed**: `day_seventeen_montages_sweep` expects the swords the other way round (the director's choice).
  It passes. No other test was run (as told: only what the change touches).

## Build

Editor module compiled. Shipping web package made twice (10.5 minutes each). No itch.io zip (not asked).

## Not done, not checked

- **Nothing was heard.** The browser reports its audio running; whether music and effects sound right, the sliders
  and the 3D placing are for the director's ears.
- **Not played to the gate**: the agent cannot steer well enough blind, and a Shipping page has no console. The
  farthest reached was the first soldiers past the start.
- Brave was not tried (the pane is Chromium).
- A click on a button less than a second after another was sometimes lost when sent by the agent's tool. Not seen
  to matter for a person; not looked into.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin and the built-in browser pane; Unreal Engine 5.9 Preview.
