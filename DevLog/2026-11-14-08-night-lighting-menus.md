# Night lighting, and the start, pause and shrine menus (2026-11-14 13:20 PST to 14:16 PST)

## When

2026-11-14 13:20 PST to 14:16 PST.

## Asked

The director wrote: "Not yet. Change the lighting to give a more night time feel while still being able to see
in-game. Add all the UI elements from the UI thread as more is finished, it may be doing it's write up right now."
("Not yet" answered the offer to package the web build.)

## What the agent did

Claude Code (Claude Fable 5.2).

- **Night.** The level's sun is now a dim blue moon that still casts the shadows; the sky is navy; the sky light is
  raised so shadows are not black; exposure is fixed so the dark stays dark; fog is dark blue. Done by
  `Tools/Level/night_lighting.py` on the level's own light actors. Two passes: the first was too dark to read the
  units, the second (kept) raised the moon, the sky light and the exposure.
- **Start menu** (new). The director's picture, the title, the tagline, "March" (Enter) and "Controls" (C). The game
  is paused behind it; it shows once for each time the game is opened.
- **Pause menu.** "PAUSED", Resume and Restart buttons that can be clicked, and the controls panel with keys and icons.
- **Shrine menu.** The designed panel: purse, a picture for each row, purple buttons, and the red "Sacrifice" button
  when the army pays.
- **Art imported**: the director's four shrine pictures and the start picture, from `E:/UE5/AI Game Jam Assets/UI`.
- Read the UI thread's two hand-off notes (`hud-handoff.md`, `menus-handoff.md`) and built from them.

## Tests

- New unit tests: the start menu's flow (March starts the run, Controls and Back); the start menu's and the pause
  menu's buttons are on the screen at four sizes, do not overlap, and are hit by a click; the level is lit as night
  (a moon of 1 to 4 lux, bluer than red, casting shadows; sky light 2 or more; fixed exposure).
- The night test was not run against the old level, because the level was changed before the build that has the
  test. The old level's numbers were read first (sun 6 lux, white, sky light 1, no post process volume): each
  would fail it.
- **Test changed**: `Hud.ShrineMenuBox` allowed a menu up to 0.6 of the screen's height; the designed panel with six
  rows is 800 of 1080, so it now allows 0.75. The room to click outside is at the panel's left.
- Unit tests: 174 passed, 0 failed.
- Scenarios: 22 passed, 0 failed (all of Tests/Scenarios).
- New scenarios: `day_fourteen_menus` (the start menu opens, its controls open, March closes it, pause, shrine) and
  `night_look` (pictures of the night).
- Screenshots looked at (`DevLog/Images/2026-11-14-08-*.png`): the fight and the gate at night (units and health
  bars read clearly, the ground is dark blue); the start menu; its controls; the pause menu; the shrine with bones
  paying and with the army paying.

## Build

Editor build: succeeded, after renaming one constant that clashed in a unity build. No web package was made.

## Not done, not checked

- Not played by hand. Clicking the menu buttons with a real mouse is untested: the scenarios use commands.
- Not seen in a browser: the night may look different there, and whether the start menu shows in the web build is
  unproven until it is packaged.
- Cinzel is not imported (it would be a download), so titles are Roboto and are not spaced out.
- The Marksman has no picture; the title is a stand-in; "Sound starts with your first click" is left off.
- No console command changes the lighting: it is the level's actors and the script.
- The final cut screen and the "gate is open" message are not designed yet by the UI thread.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview. Pictures made by the
director with fal (Nano Banana 2). The menus' design is from the "UI through Artifacts" thread (Claude Code).
