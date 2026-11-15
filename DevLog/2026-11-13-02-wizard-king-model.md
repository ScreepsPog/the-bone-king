# The Wizard King model replaces the shapes (2026-11-13 10:00 PST to 10:45 PST)

## When

2026-11-13 10:00 PST to 10:45 PST.

## Asked

The director wrote:

- "Import the model at E:\UE5\AI Game Jam Assets\Models\Wizard King, using the V2 version. All the textures are in the
  folder. It should be using the normal ue5 skeleton. Replace the current player model with that, and just use the
  default player moving animation bp. Import the started content if needed"

## What the agent did

Claude Code (Claude Fable 5.2).

- **Mannequin content.** Copied the UE5 mannequin skeleton, its unarmed animations, its default animation blueprint
  (`ABP_Unarmed`), rigs, materials and textures from the engine's template resources into
  `Content/Characters/Mannequins`. (The pistol, rifle, death, attack, dash and wall-jump animations were left out.)
- **Textures and material.** Base color, normal, roughness and metallic imported at a 1024 cap (the files are 2048),
  joined in `M_Wizard_King`. The fifth file (`_rm.PNG`, 16 MB) is roughness and metallic again and was not imported.
- **The model was not ready for the UE5 skeleton.** Its bones are named like the UE5 skeleton, but:
  - the mesh and the armature are at different scales in the file, and the whole model is about 1 unit tall;
  - every bone points along a different axis than the mannequin's bones.
  The first import crumpled into a heap as soon as a mannequin animation played (see the first screenshot the agent
  took, not kept).
- **The fix, kept as two scripts for the next Tripo character:**
  1. `Tools/Art/clean_rig_fbx.py` (Blender 5.1, no window): bakes one scale into mesh and armature and makes the model
     220 units tall. Output: `SourceArt/WizardKing/Wizard_King_V2_clean.fbx`. The director's file is untouched.
  2. `Tools/Art/import_mannequin_character.py` (Unreal, no window): imports onto the mannequin skeleton, then gives
     every bone the mannequin's orientation, swung to follow this model's own limbs. Joints and mesh stay where they
     are (read back: joints moved 0.000 units).
- **On the pawn.** A skeletal mesh under the wizard's turning part; the cylinder and sphere stay as the fallback and
  behind `Wiz.Model 0`. `ABP_Unarmed` is used as it is. It expects a Character and reads that character's movement
  component, which the wizard does not have, so the pawn writes the blueprint's four variables (velocity, ground
  speed, direction, should move) each frame. The direction is the angle between where he faces (the aim) and where he
  walks, so the blueprint picks forward, sideways or backward steps.
- **For testing.** `Wiz.Model [0|1]`, `Wiz.ModelHeight [Units=220]` (120 to 400). `Wiz.State` has `model`,
  `modelMoving`, `modelHeight`.

## Tests

- 2 new unit tests (the step angle, the height-to-scale rule), both failed against stubs first. 89 pass.
- New scenario `day_four_model`: the model is loaded, the blueprint reports moving while W is held and standing after,
  the height and the switch work. 8 scenarios pass.
- Screenshots looked at: standing, walking forward, stepping right, walking backward in the close camera; the angled
  camera.

## Build

- Shipping web package: `BUILD SUCCESSFUL` (11 minutes; the first cook of the new content).
- Loaded in the desktop app's browser pane from a local server (deferred renderer): the wizard is the textured model in
  the Outskirts, and his legs stride while W is held.
- Download: the zip went from 76.7 MB to 85.5 MB (185.5 MB unzipped, largest file 133.1 MB; the size check passes).
  The zip was made out of habit; at 10:40 PST the director said not to make the itch.io zip unless told.
- The earlier shapes build's zip was moved to `Saved/JamZips/WizardJam-web-2026-11-13-shapes.zip`.

## Follow-up (2026-11-13 11:09 PST)

The director wrote: "Go ahead and turn it into a character, it will pay off in the long run. Take a look at his lower
body as it is not moving a part of the animation bp... If it would be better, we can retarget the model to better fit."

- **Lower body.** The legs were held still by the foot placement rig inside `ABP_Unarmed`: it steers the legs toward
  helper bones (`ik_foot_l`, `ik_foot_r` and seven more) that the Tripo rig does not have. The import script now adds all
  nine, placed as on the mannequin. The legs run. Verdict given to the director: no retarget is needed; what remains is
  the robe, which is skinned as a stiff bell, so the bone legs kick out through its back at a full jog.
- **Character.** `AWizardPawn` is now an `ACharacter` (name kept). The blueprint reads his movement itself, so the code
  that fed it is removed, and with it the unit test of the step angle (the code it tested is gone). He turns as an
  actor to face the aim. The level's ground plate got collision for him to walk on, since the map's own floor is far
  smaller than a level. 88 tests and 8 scenarios pass; looked at him running in the Outskirts.

## Not done, not checked

- Not play-tested by a person.
- In the angled camera the wizard is small and dark against the ground; no light or outline was added.
- No attack, cast or death animation: Bolts leave a wizard who only stands or walks.
- Skeletons and enemies are still shapes.
- Nothing was uploaded; the director uploads to itch.io.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; Blender 5.1 (background, to
fix the file's scale); the model and textures came from Tripo (made by the director).

Screenshots: `Images/2026-11-13-02-wizard-king-idle.png`, `-step-right.png`, `-angled.png`.
