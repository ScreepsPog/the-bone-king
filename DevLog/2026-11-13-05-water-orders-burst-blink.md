# Water, smarter sent units, burst Blink, slower Bolt (2026-11-13 13:35 PST to 14:47 PST)

## When

2026-11-13 13:35 PST to 14:47 PST.

## Asked

The director wrote (the parts for this build):

- "Make it to where enemies nor allies can go on water or travel on it."
- "make it to where commanded allies to a certain location don't sit still when getting shot at. Instead, send a
  portion to attempt to deal with the attackers."
- "when a player commands the troops to an enemy checkpoint, the enemies will fight back immediately instead of
  targeting the player who may be a distance away."
- "when a group is commanded to a certain place, they don't just clump up into a group in the middle. Spread them out
  and make them idle/move around the location."
- "For the assassin, have a more gradual fade-in-out of the enemy when getting close. It should give the player more
  of a heads up while still remaining a threat."
- "Change the HUD for when you're in command mode it shows the count (amount sending) on the right of the cursor.
  Below that, display the ally type... a certain type cannot be selected unless it is a part of the current army."
- "For the blink, make it take slightly longer, and allow more control to change direction during the blink. Take a
  look at the project located at ...Nexus-part-final to see the feel I am looking for... the ability name for that is
  Dash."
- "change bone value to include the value of the current army... if I have 28 default allies... I would only get 28
  counted towards bones value. This will be used later, but not at this step."
- "change the pathing for allies to be more natural in how they get to the location, meaning not going through walls,
  using paths/shortcuts, etc."
- "tune down how fast you can fire bolt. It is currently too quick, shots should still matter."
- For a later stage, written into the design document only: an attack roots the attacker; paying at the shrine with
  army units; telegraphed move-sets; how strong the army is away from the wizard.

## What the agent did

Claude Code (Claude Fable 5.2).

- **Water.** Found by a test on the Outskirts river: where two river boxes meet, a unit pressed in by the crowd was
  pushed out of one box into the next and ended up over the water. Now it leaves by the nearest side that is clear of
  every other box, and one still inside is put back where it last stood clear.
- **Sent units.** They spread evenly over a circle sized to the group (20 units: 246 across the radius), stroll to
  new spots in it, and follow their own way around walls and water to the place (a route worked out once per order).
  Hit by an arrow from out of reach, two fifths of the group chase the shooter, up to 2,000 units from the place, and
  come back after.
- **Enemies.** With the wizard more than 700 units away, an enemy fights the nearest army unit within 350 units.
  Nearer than that, the old rule holds (they go for him), so the Overwhelm meter works as before.
- **Assassin.** Starts to show at 800 units (was 450), whole at 300. The army still sees it at 450. It is still a size
  change, not transparency.
- **Command mode HUD.** The number is right of the cursor, the kind under it; the circle at the cursor is the size the
  group will spread to. Right mouse passes over a kind the army has none of.
- **Blink.** Read from the Nexus `GA_Dash` asset: a constant force of 2,000 for 0.3 seconds along the held direction
  (the way he faces when nothing is held). Blink now starts at about 2,150 units a second and slows to a stop over
  0.65 seconds (was 0.5, eased in and out), and steers 0.6 of its range (was 0.35), evenly over its time.
- **Bolt.** One every 0.45 seconds (was 0.2). Now a `Wiz.Tune` number.
- **Bone worth.** `GetArmyBoneValue` and `GetBoneWorth`; shown in the debug stats and `Wiz.State`. Nothing spends it.
- **For testing.** `Wiz.SpawnEnemies <Count> [Kind] [X Y]`, 8 new `Wiz.Tune` numbers, 4 new `Wiz.State` fields.

## Tests

- 8 new unit tests and 4 changed ones. 11 failed against stand-ins first; 120 pass now.
- One new test passed against the old code and proves nothing new: the river made of plain slices. The test on the
  real Outskirts river is the one that failed before the fix.
- Changed because the direction changed: the Blink "eased in and out, half the time is half the way" test (now
  "starts fast and slows to a stop"); the assassin test's army part (the army's sight is its own number now); the
  input scenario's Bolt count (3 to 5 in 0.7 seconds, now 2).
- A guess of the agent's corrected: the new scenario expected two archers killed; the second walks off toward the
  wizard, which is right, so the scenario uses one. The chase limit went from 1,400 to 2,000 units after the test
  showed the chasers gave up a moment before catching a fleeing archer; and a chaser now closes inside its reach on
  an archer, because it was left one step behind forever.
- New scenario `day_six_orders`; `day_five_command` checks the kind is passed over. 12 scenarios pass.
- Screenshots looked at: `Images/2026-11-13-05-orders-spread.png`, `-command-cursor.png`.

## Build

Shipping web build packaged (`BUILD SUCCESSFUL`, 222 MB folder, 69.7 MB download) and served at `http://localhost:8900/`.
Checked in the browser pane: the game loads, and a Blink with A held, then W, moves him, slides along a house wall and
shows the cooldown. Not checked in the browser: command mode with an army, the chase, the assassin's fade, the slower
Bolt (the browser build has no console to raise an army or spawn enemies); those were checked in desktop runs.

## Not done, not checked

- Not play-tested by a person. Every new number is a first guess.
- Sent units take the shortest open way; they do not prefer roads.
- The assassin's fade is a change of size. True transparency needs a material of its own.
- Only arrows call for a chase, and only sent units answer. The army with the wizard does not chase archers.
- Enemies that kill a sent group then go for the wizard; they do not return to their post.
- No itch.io zip was made. Nothing was uploaded.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview.
