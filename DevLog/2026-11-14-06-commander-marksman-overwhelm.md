# Commander moves, the Marksman, arcing arrows, Overwhelm hold (2026-11-14 10:57 PST to 11:25 PST)

## When

2026-11-14 10:57 PST to 11:25 PST.

## Asked

The director wrote:

- "Create a new thread to work on UI through Artifacts."
- "The commander will have two move sets. 1) They have a slash that will do moderate damage in front of them. This
  happens quicker then the brute slam. It should have the same warning that the slam does of something coming so
  blink can be used. 2) They have a spear throw that targets the player, and passes through the ally army. It's main
  goal is to slow the player and give them overwhelm. The commander always tries to attack the player, and will use
  the sweep/slash attack to get through army blocking him. The commander should be the greatest threat to the player
  and have the biggest impact on overwhelm. The commander being around other units increase the rate in which they
  cause overwhelm. This needs to be displayed in an icon on the HUD."
- "As for balance, make overwhelm last slightly longer. This also means that if the player gets hit or is building
  overwhelm, it will not start to go down until 2seconds of no overwhelm build up. Also reduce the player's spear
  ability to cost the same as one base army member."
- "I want to add one final friendly AI. This will be a range enemy that targets archers first, and if none are
  present, goes for the highest health enemy. They should one shot archers. They do slightly more damage to brutes
  compared to base skeletons. Their shots go through all the targets in front to reach the designated target. Their
  arrow should also have an arc to it so it doesn't run through enemies. Do the same for the enemy archers. While it
  has a designated target, it can still hit other people if movement were to happen from the enemy."
- To import: the commander and the Marksman (with the skeleton fix), the commander's sword, a bow for the enemy
  archers, the Marksman's bow.
- Earlier, and still standing: "Do not make a build, only do the changes as I have more work to be added".

## What the agent did

Claude Code (Claude Fable 5.2).

- **UI thread.** Started a separate session, "UI through Artifacts", to design the screens as Artifact pages. It
  does not touch the code.
- **Imports (done, in the project).** Commander (286 tall) and Marksman (160 tall) through the rig clean-up and
  onto the mannequin skeleton; Commander Sword (170 long), Wood Bow (120) and Skeleton Bow (125) as meshes with
  512 textures. Nothing is held in a hand yet.
- **Commander (written).** Slash: the slam's code with quicker numbers and the same growing circle. Spear throw
  with a wind-up, through the army, slowing the wizard and filling the meter. He goes for the wizard always. On
  the wizard he counts as five; near the wizard he makes everything fill the meter 1.5 times as fast, shown by a
  crown and "x1.5" beside the Overwhelm bar.
- **Overwhelm (written).** Holds for 2 seconds after the last build-up, then drains at 0.2 a second (was 0.25).
- **Spear (written).** One skeleton, or 2 bones.
- **Marksman (written).** A new army kind that shoots from its place; a sixth row at the shrine sells it for 6
  bones (the agent's choice of how it is obtained).
- **Arrows (written).** Lobbed in an arc at where the target will be; they hit only near where they land.

## Tests

- Written before the code, `Tests/CommanderMarksmanTests.cpp`: Overwhelm holds two seconds and the commander's
  presence fills it faster; the spear costs one skeleton; the slash warns, lands in front, is quicker than a slam and
  a blink escapes it; the commander's spear passes one of the army and hits the wizard, and is not thrown at a
  wall; he goes for the wizard past the army and counts most; the Marksman kills an archer first with one arrow
  that flies over a grunt in front, then shoots the enemy with the most health, and does not walk up; arrows rise
  and land on their mark and go no farther; a Marksman is bought like a Guard.
- **Tests that were changed**: `Overwhelm.DrainsWhenClear` (asked for 0.25 gone after one clear second; now 0.2
  gone in the second after the wait). `Spear.CostsSkeletons` and `Spear.BonesFirstThenPlainSkeletonsOnly` now set
  the old price of three themselves, because they are about how the price is paid. The shrine button test now
  counts the Marksman's row as a unit row.
- **Nothing was compiled and no test was run**, on the director's instruction. Other old tests and scenarios may
  assume a spear price of three, a meter that falls at once, or a flat arrow; they will show at the first build.

## Build

None. The imports ran headless Unreal with the existing binaries.

## Not done, not checked

- Not compiled, not tested, not played. This round and the kingdom gate round are both unbuilt.
- No weapon is in any hand; the bows' origin is at one end (they went through the sword script).
- The commander and the Marksman stand through their attacks: no slash, throw or shooting animation.
- The spear throw has no mark on the ground, only his standing still for 0.7 seconds.
- The HUD icon is a stand-in drawn from rectangles.
- Balance numbers are first guesses.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview (headless Python);
Blender 5.1. The commander, Marksman, sword and bows came from the partner mesh service (Tripo), made by the director.
