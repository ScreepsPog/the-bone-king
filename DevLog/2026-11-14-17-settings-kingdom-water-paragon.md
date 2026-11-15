# Settings screen, kingdom models, the river put back, and a survey of 5,385 Paragon animations (2026-11-14 21:10 PST to 21:40 PST)

## When

2026-11-14 21:10 PST to 21:40 PST.

## Asked

The director wrote, before compacting: "Once you get the build done, implement the new settings UI that the other
thread made and Import all the meshes in this directory and their subdirectories: E:\UE5\AI Game Jam
Assets\Models\Kingdom".

Then: "Ok go ahead with what I wanted you to do. Also revert whatever water change you did. It screws up big time
around bridges. DO NOT BUILD, only do the necessary stuff. I also have all the paragon animations on my desktop,
located at C:\Users\<user>\OneDrive\Desktop\ParagonAnimationsRetargetedToManny. There 5300, way too many for me to
go through manually. I want you to go through all the animations and tell me which best (give me a few to choose
from) that match the following list I have: Death animations for all troops, friend and enemy. A falling animation
when the player gets overwhelmed. An attack animation for both the base skeleton and base brute. Attack animation
for the assassin. Spear throw forward like animation for the commander ability. Commander basic attack animation. A
crouch animation for the assassin for when he approaches. I do not want him to scale anymore, but slowly become
visible crouch running to the player, and going to normal running animation once he gets in distance. Also note
some different running, walking, and idle animations for different enemy types, but that will be an optional thing
for now. There is for sure more so if you note any, tell me them and note down the animation name. Once you find
what I am looking for, put it in a new folder in inside the folder location. If you need me to import them all to
unreal, let me know"

## What the agent did

Claude Code (Claude Fable 5.2). The director's level edits were committed first as a snapshot (`4190ca8`).

- **Water**: the cause was the last run of `Tools/Level/make_river_water.py` (in the battlefield round). The
  director had put river pieces turned sideways beside each bridge; the script drew its curve through every piece
  in order, so it zigzagged at both bridges. The river mesh was put back to the one from the commit before that
  run, and the script now skips sideways pieces. It was not run again. The five river pieces past the west edge
  therefore have no drawn water.
- **Kingdom models**: `SM_Castle_Wall`, `SM_Castle_Gate`, `SM_Castle_Gate_Door`, `SM_Castle_Tower` in
  `/Game/Kingdom`, each with its material and three textures. Import only: nothing was placed in the level, none
  has collision, and none is cut away by the cursor.
- **Settings screen**, from the UI thread's `settings-handoff.md`:
  - Opened by "Settings" on the start menu (in place of "Controls") and by a third button on the pause menu;
    "Back" or P returns to the menu it came from.
  - Controls tab: read-only. Sound tab: every row shown, none usable (the game has no sound).
  - Display tab: Quality, Render scale, HUD and menu size, Danger edges, Show frame rate work. Screen shake and
    Full screen are shown as "Later" (no camera shake in the game; full screen from a click is not wired to the
    browser and could not be checked without a web build).
  - Saved to a save slot and loaded when the game opens.
  - **Differs from the hand-off**: Quality defaults to High, not Medium, because High is what the game has run at
    until now and Medium would have changed its look without the director choosing it. The pause menu's buttons
    moved up a little and its controls panel down, to make room for the third button.
- **Paragon survey**: every one of the 5,385 FBX files was opened in Blender and its motion measured (length, how
  far the body moves and sinks, each hand's reach, speed and height, stride). The measures and the names narrowed
  the set; about 80 finalists were then drawn as strips of stick figures and looked at. 75 files were copied (not
  moved) into `_Picks_For_The_Bone_King` inside the Paragon folder, in ten sub-folders, with a `README.txt` and a
  picture of each one that was looked at. Nothing was imported into Unreal.

## Tests

- New unit tests (5, `Tests/SettingsTests.cpp`): values stay in range and on their steps; the screen opens on the
  right tab; the panel, tabs, rows, choices and sliders fit five screen sizes at three HUD sizes and are hit by
  clicks (rows that do nothing are not); the size setting scales the HUD and the tall menus still fit; settings are
  written and read back.
- One of them failed on the first run and found a real fault: rows of the Display tab overlapped by a pixel at some
  sizes. Fixed in the code.
- **Tests changed** (stated plainly):
  - `Hud.StartMenuButtons`: the second button is "Settings"; the checks of the old controls screen and its "Back"
    button are gone with that screen.
  - `Hud.PauseMenuButtons`: three buttons, "Settings" between "Resume" and "Restart the run".
  - Scenario `day_fourteen_menus`: `Wiz.StartMenu controls` now opens the settings on Controls.
  - Scenarios `day_eleven_level`, `day_twelve_placed`: 683 buildings' boxes, not 656, after the director's level edits.
- Unit tests: 200 passed, 0 failed.
- Scenarios: 28 of 29 passed. `day_two_walls` fails as before (the director's barn stands in the path it walks).
- New scenario `day_fifteen_settings`: opens from the pause menu and the start menu, each tab, each setting set and
  reported, reset.
- Screenshots looked at (`DevLog/Images/2026-11-14-17-*.png`): the three tabs; the Display tab with changed values
  at 125 % size; both bridges with the river put back.

## Build

Editor module compiled (needed to run the tests). **No web package**, as asked.

## Not done, not checked

- Not played by hand: no slider was dragged with a real mouse (clicks and drags are tested through the layout
  functions and the console only).
- Whether settings survive a browser reload in the web build is not known until a web build is made.
- Quality and render scale were set and reported, not judged by eye.
- The optional movement set and the "extras" in the Paragon picks were chosen by name and not looked at.
- The Kingdom models were looked at in Blender only, not in the level.
- `day_two_walls` still stops at the barn the director moved.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; Blender 5.1 (Kingdom
models, and measuring and drawing the Paragon animations).
