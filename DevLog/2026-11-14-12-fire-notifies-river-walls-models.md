# Fire-timed attacks, upper-body Bolt, bows, smooth river, walls, ground layers, new models (2026-11-14 17:20 PST to 18:07 PST)

## When

2026-11-14 17:20 PST to 18:07 PST.

## Asked

The director wrote: "Have the boss predict sometimes with his long straight attack. This doesn't mean they do it
every time. For brutes, remove their weakpoint indicator. Make the mouse a normal cursor when on any menu and back
to the crosshair when in game. In the editor, in landscape mode, add the two materials in /Game/Megascans/Surfaces
to be able to be painted on the ground. Additionally, turn the basic water material into a animated, water that
fits the theme. Make the river itself have rounded/smooth corners, not these harsh corners. Also make the walls
viewable in the editor as they currently are not. I replaced all the trees with a pack I had on fab. These should
not be moved. If the wall goes through, that is fine. I will adjust. I also created multiple anim montages, and
have a default anim notify named "Fire" when the attack should start. For example, if fire is called, do damage
with a sword or start the arrow shot. For Anim_Wizard_Bolt, I want you to make it to where only the upper body does
the the animation while the default running, walking, and idle still control the lower body. Attach the enemy
archer's bow named SM_Wood_Bow to the Bow_Socket. Attach the ally marksman bow named SM_Skeleton_Bow to the same
socket. Import the following weapon model: E:\UE5\AI Game Jam Assets\Models\Weapons\Basic_Steel_Hammer. How
should we manage where projectiles are fired from in this scenario? DO NOT EDIT THE MAP AS IT IS, OTHER THAN WHAT IS
SAID ABOVE. I fixed a lot of house and lamp post positioning."

Then: "DO NOT BUILD unless it is for editor reasons. I do not need to play yet"

Then: "Add the following imports: ...\Weapons\Archer_Arrow (This will be the enemy ai archer arrow),
...\Weapons\Wizard_Spear (This will be the mesh thrown during the spear ability), ...\Weapons\Marksman_Arrow
(This will be the friendly ai marksman arrow), ...\Meshes\02_Bone_Pile (Replace current dropped bones with this)"

## What the agent did

Claude Code (Claude Fable 5.2). The director's own work (trees from a Fab pack, moved houses and lamp posts, the
animations, the Megascans surfaces, the sockets) was committed first as a snapshot, as it was.

- **Commander**: half his spear throws are aimed where the wizard will be. The warning lane shows it.
- **Brute**: the glowing mark on its back is gone. The back still takes double.
- **Mouse**: pointer on menus, crosshair in the game.
- **Fire notifies**: found as montage notifies named "Fire" (archer 1.19 s, brute 0.89 s, wizard 0.80 s). The crowd's
  attack animations are lined up so Fire meets the simulation's blow; the wizard's Bolt leaves at Fire, 0.2 s after
  the cast starts.
- **Bolt on the upper body**: a C++ animation class under the mannequin's animation blueprint lays the cast over the
  spine, arms and head; the blueprint's graph was not changed.
- **Bows**: on `Bow_Socket` for the archer and the Marksman.
- **Models**: hammer imported (not held yet); the two arrows, the wizard's spear and the bone pile imported and used.
- **River**: one smooth mesh along the river's pieces, with a moving water material. The pieces stay for blocking.
- **Walls**: the four boundary walls are now actors in the level.
- **Ground**: two more paint layers from the Megascans surfaces.
- **Map**: only these were changed in the level: 4 Wall actors added, 1 "River Water" actor added, the river pieces
  rebuilt (their flat water hidden), two landscape target layers added. Nothing was moved.

## Tests

- New unit tests (7): the commander predicts some throws and not others, and a predicted one catches a wizard who
  walks straight on; the simulation tells when a blow lands (slam, archer, Marksman); each attack animation has a
  Fire notify and the animation is lined up on it; the Bolt's timing; pointer or crosshair; walls and the river
  mesh are in the level; the arrows, spear and bone pile are models of the right shape.
- Seen to fail before the work they test: the level tests (no walls, no river mesh) and the animation blueprint's
  parent class.
- **Tests changed, because the director changed the level**: `Level.APlacedBuildingBlocksItsFootprint` expected over
  120 pieces and over 100 that block; the trees are Fab meshes now, so it expects at least 60 and 40.
  `Level.TheOutskirtsIsPlacedInTheLevel` expected exactly the code's 14 posts; the level has 15, so it expects at
  least 14.
- **My own new tests corrected**: the Fire lookup (montage notifies), the wall count (the layout has 4 walls, not
  10 or more), two timing loops.
- Unit tests: 187 passed, 0 failed.
- **Scenarios changed**: `day_eleven_level` and `day_twelve_placed` (buildings 96 to 101, posts 15 to 16, pieces 141
  to 79: the level as the director left it); `day_nine_models` (swing figures 18 to 36: three more kinds animate).
- Scenarios: 24 of 25 pass. **`day_two_walls` fails**: it walks east from the start in the practice layout, and the
  barn the director moved ("House Barn 8") now stands in the way from y 504. It was not rewritten.
- New scenarios: `day_fourteen_river_bows`, `day_fourteen_arrows_bones`.
- Screenshots looked at (`DevLog/Images/2026-11-14-12-*.png`): the river from above (smooth curve, ripples); a
  Marksman holding its bow and the brute mid-slam; the wizard's spear and an arrow in flight, bows in hands; bone
  piles on the road.

## Build

Editor module compiled (needed for the Wall actors and the animation class to exist in the editor). No web package.

## Not done, not checked

- Nothing was played by hand. Not seen: the Bolt cast on the wizard (no scenario presses the mouse), the archer's
  draw up close, the mouse pointer change, the walls and paint layers in the open editor.
- The crowd's attack timing is right to within a twelfth of the attack's turn, not exact.
- Notifies on the wizard's montage other than Fire do not fire (the montage is read, not played).
- The hammer is not in the brute's hand: `Hammer_Socket` has not been placed.
- The drawn river banks and the blocking pieces differ a little at corners.
- The water's first version was drawn facing down and did not show; fixed after three looks.
- Projectiles still start at the unit's place on the ground (see the recommendation in the GDD's open questions).
- `Content/Environment_Sounds` (941 MB) is not committed to git.
- Not seen in a browser.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; Blender 5.1 (model
preparation). Models by the director (Tripo); surfaces from Megascans; trees from the director's Fab pack.
