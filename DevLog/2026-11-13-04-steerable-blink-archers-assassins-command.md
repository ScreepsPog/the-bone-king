# Steerable Blink, archers, assassins, commanding the army (2026-11-13 12:01 PST to 12:30 PST)

## When

2026-11-13 12:01 PST to 12:30 PST.

## Asked

The director wrote:

- "Continue with the rest. Change the blink to go the direction you are holding. For example, if you're holding a and
  then blink, you blink left. Allow for the player to change direction as the blink is occurring, so if I press w, it
  goes up slightly while still maintaining the original blink direction."

"The rest" is from the earlier message: archers ("shoot projectiles/arrows... more damage then a low tier guy but are
also weaker... will attempt to run when the player and army charge them"), assassins ("speedy and can charge past the
army... extremely deadly... a cloak so they cannot be seen far away"), and command mode ("F keybind... scroll wheel up
and down to determine the amount of forces... left click [to confirm]... right click, it changes the type of troop...
Don't allow commanding if no army is present... When a player comes within a certain distance of the commanded army's
location, they rejoin him. There is also a recall button").

## What the agent did

Claude Code (Claude Fable 5.2).

- **Blink.** It now goes the way the wizard is being moved (the way he faces when he stands still), its full 700
  units. A direction held during it moves him sideways 0.35 as far as he travels along it; holding back does not
  shorten it. It moves step by step now, so it slides along a wall instead of stopping dead.
- **Archer.** 6 health, an arrow every 1.6 seconds within 800 units with a clear line, 12 damage to a skeleton, 12% of
  the Overwhelm meter on the wizard. Runs when the wizard or a skeleton is within 380 units, until 570 clear. One in
  five of each post's grunts is now an archer. Arrows go through the wizard during a Blink.
- **Assassin.** 8 health, speed 540, counts as 4 enemies on the meter. Never targets the army and swerves around
  skeletons. Not drawn, and not seen by the army, beyond 450 units from the wizard. One is sent every 40 seconds
  while the commander lives.
- **Command mode.** F enters and leaves it; wheel sets the number (starts at half); right mouse steps Any, Skeleton,
  Guard; left mouse sends to the point under the mouse. Sent units hold the place, fight there, and rejoin within 300
  units of the wizard. C recalls everyone. The HUD shows the mode, the target circle, each held place, and how many
  are sent.
- **For testing.** `Wiz.Command`, `Wiz.CommandMode`, `Wiz.Recall`, `Wiz.Assassin`, `Wiz.SpawnEnemies <N> Archer|Assassin`,
  `Wiz.Blink [X Y]` (a direction now), 24 new `Wiz.Tune` numbers, 7 new `Wiz.State` fields, a debug line.
- The recall key is C: the agent's choice, since the director named no key and R restarts the run.

## Tests

- 14 new unit tests (4 archer, 3 assassin, 6 command, 1 steer) and the Blink tests rewritten, because the direction
  changed: the tests of "ends on the aim point" are gone with that rule. One key test changed for the same reason: it
  checked that F was kept free, and now checks that F commands. All new and rewritten tests failed against stand-ins
  first. 111 pass.
- One of the new tests had a wrong setup and was corrected before it ever passed: the wall in the slide test was
  short enough for the wizard to slide off its end and go around, which is the right behavior. The wall is longer now.
- New scenarios `day_five_command` (F, wheel, right mouse, left mouse, C in the running game) and `day_five_archers`
  (arrows hit the wizard). `day_five_blink` changed with the command's meaning. 11 scenarios pass.
- Screenshots looked at: `Images/2026-11-13-04-command-mode.png`, `-command-sent.png`, `-archers.png`, `-assassins.png`.

## Build

- Shipping web package: `BUILD SUCCESSFUL`.
- Loaded in the desktop app's browser pane from a local server, in the Outskirts: holding A and pressing Space, then
  holding W, carried the wizard a long way left and somewhat up, and the HUD counted the cooldown. F with no army did
  nothing, as intended.
- Not checked in the browser: command mode with an army, archers and assassins. The browser build has no console to
  add skeletons or enemies with, and reaching them means playing through a post. They are covered by the desktop
  scenarios and screenshots.
- No zip.

## Not done, not checked

- Not play-tested by a person. Every number for the new enemies and for command mode is a first guess.
- Sent units walk straight at their place and can be held up by a wall between.
- Archers and assassins are cones; arrows are plain shafts. No sound, no effects, no cloak effect beyond growing into view.
- The Practice map's waves do not include the new enemies; they come from posts and the assassin timer in the Outskirts.
- No itch.io zip was made. Nothing was uploaded.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview.
