# The Bone King

A solo entry (Screeps) for the 2026 Epic AI MegaJam, theme "Director's Cut". You are the villain: a wizard king who
raises the dead, builds an army of skeletons and takes back the kingdom. Built in Unreal Engine 5.9 and played in a
web browser.

This repository is the jam's open dev log: the code, the tools, the tests and a written record of every work session.

## What is here

| Folder | What it holds |
|---|---|
| `DevLog/` | One entry per work session: what was asked, what was done, what failed, tests, builds. Pictures in `DevLog/Images/`. |
| `docs/GDD.md` | The game design document: the single source of direction, with a decisions log. |
| `docs/WhatWasUsed.md` | Every tool, service and asset source used, and what was made before the jam. |
| `Source/` | The game's C++ code and its automated tests. |
| `Tests/Scenarios/` | Scripted play-throughs run against the game. |
| `Tools/` | Python tools for the level, art, audio and the jam build. |
| `Plugins/ClaudeTestHelper/` | Test helper used by the automated game tests. |
| `Config/`, `WebTesting.uproject` | Project settings. |

## What is not here

The `Content` folder (models, textures, animations, sounds, the level) is not in this repository. Much of it comes
from asset packs and services whose licences do not allow passing the files on. Without it the project compiles but
does not open as the finished game. `docs/WhatWasUsed.md` lists where each part came from.

## Dev log, oldest first

| Date | No. | Entry |
|---|---|---|
| 2026-11-11 | 01 | [Declared pre-jam work](DevLog/2026-11-11-01-declared-pre-jam-work.md) |
| 2026-11-11 | 02 | [Day one core: a playable loop in shapes](DevLog/2026-11-11-02-day-one-core.md) |
| 2026-11-12 | 01 | [Base feel: an army that stays close, enemies that charge, walls](DevLog/2026-11-12-01-base-feel.md) |
| 2026-11-12 | 02 | [Web playtest by the agent](DevLog/2026-11-12-02-web-playtest.md) |
| 2026-11-12 | 03 | [Army groups and the shaking at the leash](DevLog/2026-11-12-03-army-groups.md) |
| 2026-11-12 | 04 | [Bones, even groups and health bars](DevLog/2026-11-12-04-bones-groups-health-bars.md) |
| 2026-11-12 | 05 | [Test zip for itch.io](DevLog/2026-11-12-05-itch-test-zip.md) |
| 2026-11-12 | 06 | [Bone spear, the Brute, the Guard, and a plan for the map](DevLog/2026-11-12-06-spear-brute-map-plan.md) |
| 2026-11-12 | 07 | [Shrine, pause menu, bone values, and the stuck key](DevLog/2026-11-12-07-shrine-pause-bone-values.md) |
| 2026-11-12 | 08 | [Summon the Afterlife, the shrine shop, and the new direction](DevLog/2026-11-12-08-summon-spell-shrine-shop.md) |
| 2026-11-12 | 09 | [Research, half-speed shrine, garrisons, and the Outskirts block-out](DevLog/2026-11-12-09-research-garrisons-outskirts-map.md) |
| 2026-11-13 | 01 | [Bolt range, a kill wakes the group, life at posts, road patrols, larger scale](DevLog/2026-11-13-01-bolt-range-living-posts-road-patrols.md) |
| 2026-11-13 | 02 | [The Wizard King model replaces the shapes](DevLog/2026-11-13-02-wizard-king-model.md) |
| 2026-11-13 | 03 | [Unit sizes, Blink, one key per control](DevLog/2026-11-13-03-sizes-blink-one-key-controls.md) |
| 2026-11-13 | 04 | [Steerable Blink, archers, assassins, commanding the army](DevLog/2026-11-13-04-steerable-blink-archers-assassins-command.md) |
| 2026-11-13 | 05 | [Water, smarter sent units, burst Blink, slower Bolt](DevLog/2026-11-13-05-water-orders-burst-blink.md) |
| 2026-11-13 | 06 | [Skeleton model, the wizard's presence, and seven fixes](DevLog/2026-11-13-06-skeleton-model-presence-fixes.md) |
| 2026-11-13 | 07 | [New sizes, see-through buildings, the army pays at the shrine, one-blow melee](DevLog/2026-11-13-07-sizes-see-through-army-pays-one-blow.md) |
| 2026-11-13 | 08 | [Grunt and Guard models, timed swing, spear paid in bones, brute's weak back](DevLog/2026-11-13-08-models-swing-spear-brute.md) |
| 2026-11-13 | 09 | [The browser hitch: found and fixed](DevLog/2026-11-13-09-browser-hitch-skin-cache.md) |
| 2026-11-13 | 10 | [Brute slam, melee that stands, swing fix, new prices, assassin model, houses and shrine](DevLog/2026-11-13-10-slam-rooted-melee-assassin-buildings.md) |
| 2026-11-13 | 11 | [The Outskirts as a real level, archer and brute models, sharper buildings, Fab library](DevLog/2026-11-13-11-outskirts-level-archer-brute-fab.md) |
| 2026-11-14 | 01 | [The slam spares armor and guards; two swords imported](DevLog/2026-11-14-01-slam-spares-armor-swords.md) |
| 2026-11-14 | 02 | [Enemies lose sight of the wizard, search, and go back](DevLog/2026-11-14-02-enemies-lose-sight.md) |
| 2026-11-14 | 03 | [Level pieces as actors, mesh houses, and a landscape ground](DevLog/2026-11-14-03-level-pieces-landscape.md) |
| 2026-11-14 | 04 | [The level laid out in the editor: shrine, posts, houses, lamps](DevLog/2026-11-14-04-level-laid-out-in-editor.md) |
| 2026-11-14 | 05 | [One level, and it ends at the kingdom gate](DevLog/2026-11-14-05-one-level-kingdom-gate.md) |
| 2026-11-14 | 06 | [Commander moves, the Marksman, arcing arrows, Overwhelm hold](DevLog/2026-11-14-06-commander-marksman-overwhelm.md) |
| 2026-11-14 | 07 | [Spear throw warning, the designed HUD, and the first build of three rounds](DevLog/2026-11-14-07-throw-warning-hud-first-build.md) |
| 2026-11-14 | 08 | [Night lighting, and the start, pause and shrine menus](DevLog/2026-11-14-08-night-lighting-menus.md) |
| 2026-11-14 | 09 | [Cinzel for the titles](DevLog/2026-11-14-09-cinzel-titles.md) |
| 2026-11-14 | 10 | [The final cut screen and the gate message](DevLog/2026-11-14-10-final-cut-gate-message.md) |
| 2026-11-14 | 11 | [Web package with the night, the menus and the final cut](DevLog/2026-11-14-11-web-package-menus-night.md) |
| 2026-11-14 | 12 | [Fire-timed attacks, upper-body Bolt, bows, smooth river, walls, ground layers, new models](DevLog/2026-11-14-12-fire-notifies-river-walls-models.md) |
| 2026-11-14 | 13 | [Throw arm, painted roads, bridge model, better water, hammer, commander's spear, tighter houses](DevLog/2026-11-14-13-bridge-water-throw-arm.md) |
| 2026-11-14 | 14 | [Web package with the bridge, water, models and attack animations](DevLog/2026-11-14-14-web-package-bridge-water.md) |
| 2026-11-14 | 15 | [Battlefield edge, Marksman behaviour, your collision, see-through, and why the web build had no bows](DevLog/2026-11-14-15-battlefield-marksman-collision.md) |
| 2026-11-14 | 16 | [Web package with the battlefield edge, Marksman changes and the missing animations and weapons](DevLog/2026-11-14-16-web-package-battlefield.md) |
| 2026-11-14 | 17 | [Settings screen, kingdom models, the river put back, and a survey of 5,385 Paragon animations](DevLog/2026-11-14-17-settings-kingdom-water-paragon.md) |
| 2026-11-14 | 18 | [Changeable keys, sound channels, the director's sounds and the Paragon picks imported](DevLog/2026-11-14-18-keys-sound-channels-paragon-import.md) |
| 2026-11-14 | 19 | [The chosen animations, music, sound effects, the bridge's collision and darker leaves](DevLog/2026-11-14-19-animations-music-sound-bridge-leaves.md) |
| 2026-11-15 | 20 | [The director's montages, weapons in hand, the brute's sweep, shadows and the thud's timing](DevLog/2026-11-15-20-montages-weapons-sweep-shadows.md) |
| 2026-11-15 | 21 | [Bodies hold their last frame, sounds placed in 3D, volumes applied by the game](DevLog/2026-11-15-21-bodies-hold-3d-sound-volume.md) |
| 2026-11-15 | 22 | [Web build of the last four rounds, swords swapped, settings kept in the browser](DevLog/2026-11-15-22-web-build-swords-saves.md) |
| 2026-11-15 | 23 | [The Bolt of fire, glowing marks on the ground, music at no volume, the padlock, a smaller download](DevLog/2026-11-15-23-fire-bolt-ground-marks-smaller.md) |
| 2026-11-15 | 24 | [Three more sounds, a light fog, the gate's ground moved nearer](DevLog/2026-11-15-24-three-sounds-fog-gate.md) |
| 2026-11-15 | 25 | [The Bolt through houses, the spear through the army, fewer plugins, a web build and its zip](DevLog/2026-11-15-25-bolt-spear-plugins-web-build.md) |
