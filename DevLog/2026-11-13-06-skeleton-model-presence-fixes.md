# Skeleton model, the wizard's presence, and seven fixes (2026-11-13 14:50 PST to 15:50 PST)

## When

2026-11-13 14:50 PST to 15:50 PST.

## Asked

The director wrote:

- "Allow for more control earlier on in the blink. Also, for now, have the character disappear on blink and reappear
  as easing at the end occurs."
- "have fallen friendly ai drop a portion (1/3) of their cost back. For example, a base skeleton is 1, and a guard is
  3 (rounded from 2.67). Ensure it's always rounded is a whole number."
- "Have archers attempt to shoot where the player is moving towards... Also make it to where it isn't always
  perfect, so there is some forgivability, but not too much."
- "Don't have the AI fight one another (unless they're enemies vs friendly) in terms of trying to move... Either pick
  a different spot or find another route that doesn't push each other."
- "Don't have bones despawn while in camera view... As soon as bones go out of camera view, have a set time before
  they despawn."
- "fix friendly army ai to not jitter/move back and forth really quick when near a wall or the river... Have the
  rather go to an open area where enemies can come from."
- "Replace the default skeleton simple shapes with the model found here: ...Friendly_AI_Army\01_Base... Ensure it has
  the correct skeletal bones and stuff so animation works, like the player character... Use the same abp as the
  player for now. If you need to bake the animation... go ahead and do that. Note to change this later. IMPORTANT:
  Ensure the size matches the scale that was discussed earlier. Keep this in mind for all character model imports
  going forward."
- "Let's go ahead with the option 1 for the army strength and keep the commander in mind if time permits."
- Written into the design document, not built: paying at the shrine with the army (bones first; no bones left by the
  units spent), and kinds of ally that give the wizard bonuses.

## What the agent did

Claude Code (Claude Fable 5.2).

- **Skeleton model.** The partner mesh (1,141 vertices, mannequin bone names) went through the wizard's route: Blender
  bakes it to 66 units tall, Unreal imports it onto the mannequin skeleton and turns its bones to the mannequin's.
  Plain skeletons in the army are now this model, at 0.3 of the wizard's height. No baking was needed: three hidden
  figures run the player's animation blueprint (standing, strolling, running) and every skeleton copies one of them.
  The cost of that choice: all that stroll step in time with each other. Guards are still shapes.
  First try they all crouched: the blueprint plants feet on the ground under the figure, and the hidden figures were
  not standing on any. They now stand where the wizard is.
- **Presence.** Beyond 600 units from the wizard an army unit does half damage and takes half as much again.
- **Blink.** Six tenths of the steering now follows the burst, so the start steers hardest. He is not drawn from the
  start until seven tenths of the time have passed.
- **Fallen allies.** A skeleton leaves 1 bone, a Guard 3. Units spent on the spear leave none.
- **Archers.** They aim at where the target will be, from how it moves at that moment, and each arrow is off by up
  to 4 degrees.
- **No shoving.** A walking unit goes around a standing one of its own side; the standing one is not pushed; a taken
  spot is given up for another.
- **Walls and water.** A skeleton whose spot ends up in or beyond a wall or the river takes one on open ground on the
  wizard's side.
- **Bones.** A pile the camera shows does not fade. Its time starts when it leaves the screen.
- **For testing.** `Wiz.ArmyModel [0|1]`, 8 new `Wiz.Tune` numbers, 5 new `Wiz.State` fields, three debug lines.

## Tests

- 10 new unit tests (9 on behavior, 1 on the model's size and skeleton). All failed first; 130 pass.
- A wrong setup of the agent's, corrected: the presence test's skeleton far from the wizard was not sent anywhere, so
  it would not fight at all. It is sent to its spot now.
- A rule of the agent's that broke an old test and was changed, not the test: "a taken spot means stay put" stopped
  the army strolling. A unit now picks another spot first.
- New scenario `day_seven_army`; `day_five_blink` checks he is hidden, then seen. 13 scenarios pass.
- Screenshots looked at: `Images/2026-11-13-06-skeleton-models-close.png`, `-skeleton-models-run.png`, `-army-models.png`.

## Build

Shipping web build packaged (`BUILD SUCCESSFUL`, 223 MB folder) and served at `http://localhost:8900/`.
Checked in the browser pane: the game loads and a Blink to the right moves him and starts the cooldown. Not seen in
the browser: the moment he is hidden (the screenshot came after he was back), the skeleton models (the run starts
with no army and the browser build has no console to raise one), and everything else in this round. Those were
checked in desktop runs and screenshots.

## Not done, not checked

- Not play-tested by a person. The new numbers are first guesses.
- The frame rate with forty skeleton models was not measured in the browser.
- Strolling skeletons step in unison, and switching between standing, strolling and running is a snap, not a blend.
- The feet slide: the mannequin's stride is made for a figure three times as tall.
- Blink has no effect yet; he is simply not drawn.
- Paying at the shrine with the army, and bonuses from kinds of ally, are not built.
- No itch.io zip was made. Nothing was uploaded.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; Blender 5.1 (rig cleanup).
The skeleton mesh and its textures came from the partner mesh service (Tripo), made by the director.
