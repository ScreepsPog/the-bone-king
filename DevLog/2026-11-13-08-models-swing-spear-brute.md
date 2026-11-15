# Grunt and Guard models, timed swing, spear paid in bones, brute's weak back (2026-11-13 16:40 PST to 17:34 PST)

## When

2026-11-13 16:40 PST to 17:34 PST.

## Asked

The director wrote:

- "For spear of sacrifice, make it to where it can only take existing skeleton members, not any other type. It will
  also now take the bone currency by default rather then taking the army first. If there is not enough currency, it
  will then call a slash animation at a couple of troops to create the spear."
- "add the unarrmed swing animation that comes with ue5 packs for melee attacks. This will be temporary. Line up
  damage to take place on swing connect."
- "fix enemy ai from overlapping one another at checkpoints. This was not a issue prior due to sizing".
- "change the brutes to have their weak point at their back side and are more tanking in the front."
- "As for the curve graph, create an artifact for this chat real quick to manage all of these stats. Make it look good."
- "Import the following and do the rig fix": the grunt model (Enemy_AI\01_Base) and the Guard model
  (Friendly_AI_Army\02_Guard).

## What the agent did

Claude Code (Claude Fable 5.2).

- **Models.** The Guard (175 units) and the grunt (170) went through the skeleton's route and are drawn in the game.
- **Swing.** The engine's unarmed attack (`MM_Attack_01`, one second, fist farthest out at 0.45 to 0.5 seconds) was
  copied into the project and its forward step locked out. A striking figure copies one of six copies of the swing,
  chosen so the fist is out when its damage lands; a test shows the worst case is a twelfth of a second off.
- **Spear.** 4 bones when he has them (the agent's guess at a price); otherwise 3 plain skeletons with him, each
  shown with a pale cut. Guards are never taken. The wizard himself has no slash animation yet.
- **Posts.** Units now take the room of their bodies, so reach, the wizard's own body and the touch range had to
  follow. At a post a unit will not stroll onto an occupied spot, and two that cross are put clear of each other.
- **Brute.** Half damage from the front, double from the back, full from the side; it turns at 140 degrees a second.
  A glowing knot marks its back.
- **Curve editor.** Published as a private page: https://claude.ai/artifact/3cYxQELdq1HW66FRGzGqUd

## Tests

- 6 new unit tests (spear payment, post spacing, brute, strike clock, the two models' size, swing timing). The four
  on behavior failed against stand-ins first; 139 pass.
- The post spacing test was changed before it ever passed: it first demanded that no two units overlap by a tenth at
  any moment, which two walkers brushing past cannot meet. It now demands that no two standing units overlap by a
  tenth and nobody passes through another by a quarter.
- Changed because the sizes changed: the group tightness limit (140, now the shuffle distance plus 80) and the reach
  in the wall test (450, now the roam circle plus 100).
- Two tries of the agent's that were undone: putting a walker fully clear of a stander made both jitter, and broke
  the no-shoving test. The softened push stays.
- A scenario mistake of the agent's: `Wiz.Buy Spear` with no bones now buys with the army, which emptied the test army.
- New scenario `day_nine_models`. 15 scenarios pass.
- Screenshots looked at: `Images/2026-11-13-08-models-fight.png`. The swing itself was not judged by eye in motion.

## Build

Shipping web build packaged (`BUILD SUCCESSFUL`, 225 MB folder) and served at `http://localhost:8900/`.
Checked in the browser pane: the game loads; walking up the road, a patrol of five grunts came at the wizard drawn as
the grunt model, spaced apart, and the Overwhelm meter began to fill. Not checked in the browser: the swing in motion,
the Guard and skeleton models (no army without a console), the spear's payment and the brute's back; those were
checked in desktop runs.

## Not done, not checked

- Not play-tested by a person. Nobody has watched the swing in motion to judge how it reads.
- The frame rate with up to 140 models in the browser is not measured.
- Fewer enemies fit around the wizard now that they are larger, so Overwhelm fills more slowly than before. Not retuned.
- The spear's bone price (4) is a guess.
- The curves are not in the game.
- No itch.io zip was made. Nothing was uploaded.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; Blender 5.1 (rig cleanup).
The grunt and Guard meshes and textures came from the partner mesh service (Tripo), made by the director.
