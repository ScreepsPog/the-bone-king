# Battlefield edge, Marksman behaviour, your collision, see-through, and why the web build had no bows (2026-11-14 19:40 PST to 20:28 PST)

## When

2026-11-14 19:40 PST to 20:28 PST.

## Asked

The director wrote: "The archers are not doing the right animation nor holding the bow when shooting their arrows.
They use the basic unarmed attack. The same goes for the marksman. They should both use the same animation but use
their respective bows. Additionally, when marksman shoot in the friendly army, make them stand in place, meaning
they cannot move with the player when shooting. They will shoot, then catch up to the player. Marksman should also
try to be far from the enemy. If an enemy get's close they'll try to relocate, but not always. Also don't allow for
guards/marksman to be converted in the army, nor count towards the bone currency. Also fix it show the cursor can
see through the trees and new houses like it did prior to addition. However, it should no longer work on the wall
or the gate mesh when it's added. Also change the AI to walk on my collision boxes for the bridge and ensure they
still correctly go around houses as I also updated those collision boxes. Replace the walls with just a soft
barrier, that warns the player to return to the battlefield, with a failing of the mission if they wait to long or
go to far. Since this changes the bound, add some additional houses/lamp posts/water and such. I will do the
landscape painting as I see fit here. Do not do it for the kingdom wall side as I will attempt to make a castle
walls and castle gate here. Import this weapon: E:\UE5\AI Game Jam Assets\Models\Weapons\Basic_Dagger"

Through the assets thread (not typed here): the game is called "The Bone King"; and a request to import Paragon
animation packs from Fab, which was **not done** (it needs the director's own Epic sign-in and direct OK).

## What the agent did

Claude Code (Claude Fable 5.2). The director's collision and level edits were committed first as a snapshot.

- **Bows and animations in the web build**: the cause was the package, not the game. Their folders were missing
  from the cook list, so the browser build fell back to the unarmed swing and plain shapes. Added.
- **Marksmen**: stand for the draw, catch up after the arrow; back away from a close enemy 60% of the time.
- **Sacrifice and bone worth**: plain skeletons only.
- **See-through**: houses and the tree pack's materials are cut away around the cursor; walls no longer are.
- **Collision**: houses block the director's convex pieces; the bridge's deck is walked on, its rails block, and
  figures stand at the deck's height.
- **Battlefield**: walls deleted, one Battlefield actor in their place, warning and fail; 21 houses, 16 lamp posts
  and five river pieces added past three edges. Nothing east. No landscape paint was changed.
- **Dagger** imported.

## Tests

- New unit tests (5): the battlefield's warning and both ways of failing; boxes from collision shapes (a turned
  house, a bridge's rail and deck, and the director's own bridge and barn meshes); the Marksman stands, catches
  up, relocates or does not; only skeletons pay or count; the level has a battlefield, no walls, the start and the
  gate inside it, dressing past the edge and none to the east.
- **Tests changed for the director's new rules**: `Bones.TheArmyCountsTowardBoneWorth` (three Guards added 9 to the
  army's worth; they add nothing now), `Shrine.SkeletonsPayBeforeGuardsAndSentUnitsDoNot` (Guards paid after
  skeletons; they never pay now), `Level.WallsAreActorsInTheLevel` (expected the four walls; expects none now).
- **My own test corrected**: a point exactly on the line between two boxes is not inside either.
- First version made 4,372 blocking boxes for the houses (one grid for each collision piece). Changed to one grid
  for each building with rows merged: 656.
- Unit tests: 195 passed, 0 failed (the lamp post by the shrine no longer trips its test).
- Scenarios: 27 of 28 pass. `day_eleven_level` and `day_twelve_placed` were changed again (building boxes 343 to 656, pieces 46 to 57: the level as it is now). `day_two_walls` still fails at the director's barn.
- New scenarios: `day_fourteen_battlefield` (warning on leaving, gone on return, mission failed when the time runs
  out), `day_fourteen_seethrough_bridge`.
- Screenshots looked at (`DevLog/Images/2026-11-14-15-*.png`): a house cut away around the circle; figures and the
  wizard standing on the bridge's deck; the "RETURN TO THE BATTLEFIELD" warning with its countdown.

## Build

Editor module compiled. Web package: see the next entry.

## Not done, not checked

- Not played by hand. The Marksman's standing and backing away were tested by numbers, not watched.
- The see-through was seen on a house, not on a tree.
- Whether enemies path cleanly around every house with the new boxes was not watched; `day_two_walls` still stops
  at the barn the director moved.
- The archer's and Marksman's bows and shoot animation in a browser: fixed by the cook list, confirmed only once
  the new package is played.
- The Paragon packs were not imported.
- The edge dressing is placed by a seeded random scatter; the director may want to move or delete pieces.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; Blender 5.1 (dagger).
