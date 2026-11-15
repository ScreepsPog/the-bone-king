# One level, and it ends at the kingdom gate (2026-11-14 10:39 PST to 10:55 PST)

## When

2026-11-14 10:39 PST to 10:55 PST.

## Asked

The director wrote:

- "I am gonna condense this project down to just being the one level considering the remaining time left as we still
  have sound, vfx, and other things to complete in less than 30 hours"
- "The game finishes when they enter the kingdom gates."
- Asked whether the gate should open only after the commander is dead: "Yes, commander first"
- "Do not make a build, only do the changes as I have more work to be added"

## What the agent did

Claude Code (Claude Fable 5.2).

- **Design document.** The game is the Outskirts alone; Inside the Walls, the Castle and the Tyrant are cut. The
  commander is the last enemy, and the game finishes at the kingdom gate once he is dead.
- **The gate, written.** A Kingdom Gate actor for the level (width, depth, a marking on the ground that shows once
  it is open), read by the game like the shrine and the posts. The gate's rule is plain data with its own tests.
- **The ending, written.** When the commander dies the screen says the gate is open and keeps "Enter the kingdom
  gate" up. Walking in stops the fight and shows the final cut screen: time, skeletons raised and sacrificed,
  enemies killed, the army's size, and "R: march again".
- **For testers.** `Wiz.OpenGate`, `Wiz.Goto gate`, and `gate` in `Wiz.State`.
- **Placing it.** `Tools/Level/place_gate.py` puts the gate into the level, 900 units past the commander's post.
  It needs the new code built first, so it has not been run.

## Tests

- New unit tests, written before the code: the gate is shut while the commander lives; entering it once he is dead
  ends the game, once; a turned gate is entered where it stands; the Outskirts layout has a gate in the open that can
  be walked to from the commander.
- **A test was changed**: the placed-level test now also asks for exactly one Kingdom Gate actor in the level, in
  the open, past the commander's ground. It will fail until the gate is placed.
- **Nothing was built and no test was run**, on the director's instruction. The code has not been compiled.

## Build

None.

## Not done, not checked

- Not compiled, not tested, not played.
- The gate is not in the level. The spot chosen from code (900 units past the commander) has not been checked
  against the houses that now stand there; the tests will say after a build.
- The gate has no model, and a shut gate does not block: it is ground, with the shrine's ring stretched over it.
- No scenario plays the ending yet.
- The fall still restarts the run with no final cut screen. Perks and detonations are not on the screen.

## Tools used

Claude Code (Claude Fable 5.2). No engine run.
