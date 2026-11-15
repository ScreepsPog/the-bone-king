# Shrine, pause menu, bone values, and the stuck key (2026-11-12 15:25 PST to 15:28 PST)

## When

2026-11-12 15:25 PST to 15:28 PST.

## Asked

- The spear is called the "Spear of Sacrifice".
- "I want each enemies death and their bone value they drop to have a different value... a small guy should still
  be 1, but a brute should be like 3... we will need to rethink how armies are created in terms of bones and how
  they convert with different types of friendly AI."
- "I also don't want this to just be a endless wave clear type game, there should be a set amount per round with a
  powerful boss/general/commander at the end to cap it." On the shrine: "I am leaning towards all round when waves
  will die down for a moment, before reinforcements come back and attack again."
- "You're good to build a shrine." With "a basic pause menu and a placeholder menu for the shrine when opened. Game
  still runs while in shrine and movement is not locked. You are kicked out of the shrine if you go out of it's
  range. However, you cannot do manual abilities during shrine usage. Allow for the menu to be closed by pressing
  escape, going too far away, or clicking outside the menu box."
- A bug: "if you right click the page while moving, the key gets stuck."

## What the agents did

Claude Code (Claude Fable 5.2) wrote the tests first, built it, packaged the web build and checked it in the browser
pane. Commit `ccfc58e`.

- **Shrine.** A dais with a glowing pillar and a circle on the ground (230 units), ahead and to the right of the
  start. In the circle the HUD says "F: use the shrine". F opens a placeholder menu. While it is open the fight goes
  on, the wizard can walk, and Bolt and the Spear of Sacrifice do nothing. It closes on Esc or F, on a click outside
  its box, on walking out of the circle, and when the wizard falls.
- **Pause menu.** Esc or P pauses and resumes; R restarts the run while paused. Esc closes the shrine's menu first.
- **Bone values.** Every kind of enemy leaves its own number of bones in one pile: grunt 1, Brute 3. A Brute's pile
  is drawn larger.
- **Name.** The HUD and the commands say "Spear of Sacrifice".
- **The stuck key.** Cause: a right click opened the browser's own menu, which took the keyboard, so the release of
  a held key never reached the game. Fix: a small page script (`Build/Wasm/Template/shell/game-input.js`) that
  stops that menu, and also releases every held key when the page loses the keyboard for any other reason.
- **For testing.** `Wiz.Shrine [0|1]`, `Wiz.Pause [0|1]`, `Wiz.AddSkeletons <Count> [Skeleton|Guard]`,
  `Wiz.Tune BonesPerBrute`.

**A consequence to decide.** A Brute used to leave a pile that became a Guard. Now it leaves 3 plain bones, so
nothing in the game makes a Guard any more. The Guard is still built and can be added with the command. How bones
become the different kinds of skeleton is the open question. Proposals:

1. **Choose at the shrine** (recommended). Bones only pile up; the shrine's menu is where they are spent: a
   skeleton for 3, a Guard for 9, later kinds for more. It gives the shrine a job at once and makes walking to it
   under attack the risk.
2. **Automatic, biggest first.** Bones assemble the best unit they can pay for when a threshold is passed. Nothing
   to learn, but no choice either.
3. **A switch on the HUD.** One key chooses what bones assemble into, anywhere. A choice without the shrine, but one
   more key in a fight.

**Rounds: a first proposal, not built.** A round is three assaults. An assault is a set number of enemies in a few
waves; when the last of them is dead there is a lull of about 10 seconds, shown on the HUD as a countdown to the
reinforcements. That lull is the moment for the shrine. After the third assault the commander arrives with an
escort; killing him ends the round. Each round is larger than the last.

## What failed and the fix

- **The first shrine menu covered the wizard.** It was centered, as my own test demanded, and the screenshot showed
  the wizard, who can still be attacked, hidden behind it. The box now stands at the right of the screen and the
  test was changed to demand that the wizard is not covered. One line of its text also ran past the box; the lines
  were shortened.
- **A Brute's test had to change** because the direction changed: it asserted that a Brute's bones make a Guard.
- **An edit script stopped half way** on a wrong indentation again. Fixed and rerun.

## Screenshots

- `Images/2026-11-12-shrine-menu.png`: desktop, the shrine, its circle and the menu at the right.
- `Images/2026-11-12-browser-shrine.jpg`: the web build with the menu open while grunts close in.

## Web package

Development build, `BUILD SUCCESSFUL`, 291.0 MB staged, `.wasm` 192.0 MB. The page script is staged and loaded.

Browser pane check: the right-click menu no longer opens (the page reports it blocked, and a real right click
showed none); F opens the shrine in its circle; the wizard was overwhelmed while the menu was open, which closed
it; P pauses, Esc resumes. 120 fps.

Tests: 6 new ones failed against the old code first. 61 pass, plus five key-press scenarios (`day_two_shrine` is
new: it walks into the circle, presses F, tries the spear, presses Escape, and walks out).

## Not checked

- The stuck key in Brave by hand, with a real held key and a real right click.
- Esc in full screen: a browser uses Esc to leave full screen first, so P is the dependable pause key there.
- A click outside the menu by hand (covered by the box test and the code path, not by a scripted click).

## Tools used

Claude Code with Claude Fable 5.2, Unreal Engine 5.9 Preview, the Claude app's browser pane.
