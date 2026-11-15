# New sizes, see-through buildings, the army pays at the shrine, one-blow melee (2026-11-13 15:50 PST to 16:38 PST)

## When

2026-11-13 15:50 PST to 16:38 PST.

## Asked

The director wrote, with a table of heights (wizard 220, plain skeleton 150, Guard 175, grunt 170, brute 240):

- "Use this (image 1) as scale. The skeletons right now are too small."
- "Make it to where when a cursor is over a building, the building loses opacity within a circle around the cursor.
  This is for when you want to aim at enemies behind said buildings but can't see them."
- "Go ahead and start building the paying at the shrine with the army part. As for the guard bonuses, I like the idea
  of create a curve graph for these types of stats. This means they're good up to a certain amount, then the trade
  off is bad."
- "Also change all melee attacks to be one hit rather than a continuous hit for both enemies and friendly AI. When a
  enemy AI reaches the player, keep the the slowly building up overwhelm for now."

## What the agent did

Claude Code (Claude Fable 5.2).

- **Sizes.** The table's heights are in. Not in the table, chosen by the agent: archer and assassin 165, commander
  286. The skeleton model was imported again at 150 units. Because the figures are more than twice as large, the
  agent also widened the crowd: 70 units between units (was 50), groups 280 from the wizard with 100 of shuffle
  (240 and 60), roam circle 420 (350).
- **See-through.** A new material for walls, houses, trees and tree tops: inside a circle around the cursor on screen
  (0.13 of the screen's height) the surface is cut to a speckle of about one pixel in eight. `Wiz.SeeThrough`.
- **Shrine.** Bones first, then plain skeletons (1 each), then Guards (3 each), nearest the wizard first; they leave
  no bones. The menu shows what the army can pay and prints a row's price in red when the army would pay part.
- **Melee.** One blow a second for a second's worth of damage, the first half a second after getting in reach.
  Enemies on the wizard fill Overwhelm steadily as before.
- **Guard bonus curve.** Written into the design document as a proposal. Not built.

## Tests

- 3 new unit tests (shrine payment, its order and limits, one-blow melee). They and two changed tests failed first;
  133 pass.
- Changed because the direction changed: the unit size test (the first sizes are gone); the bone worth test (three
  Guards count 9, not 8, now that each unit counts a whole number); the spear shop test ("11 bones cannot buy it"
  is no longer true beside five skeletons, so it uses 5 bones).
- A rule added after a scenario showed a problem: "Max" Guards with 28 bones and 4 skeletons bought four and
  sacrificed the skeletons without showing it. The army now only ever pays the rest of one purchase. A test covers it.
- The see-through circle has no unit test: it is a material. It was checked by picture (`wall-whole`, `wall-cut`).
- New scenario `day_eight_shrine_army`; the height checks in `day_five_blink` follow the table. 14 scenarios pass.
- Screenshots looked at: `Images/2026-11-13-07-wall-cut.png`, `-shrine-army-pays.png`, `-sizes-new.png`.

## Build

Shipping web build packaged (`BUILD SUCCESSFUL`, 223 MB folder, 70.4 MB download) and served at `http://localhost:8900/`.
Checked in the browser pane: the game loads, and with the cursor over a house the house is cut away in a circle
around it, with a soft edge, showing the ground behind. Not checked in the browser: the new sizes on an army, the
shrine payment and the one-blow melee (the run starts with no army and the browser build has no console); those were
checked in desktop runs and screenshots.

Also recorded in the design document at the assets session's request (relayed from the director): the wizard is the
villain and the "tyrant" is the good king; the art route for characters.

## Not done, not checked

- Not play-tested by a person.
- The see-through circle is a speckle, not smooth transparency.
- Enemy shapes (boxes, cones) are now large; only the plain skeleton has a model.
- A Guard that pays more than is owed gives nothing back.
- The Guard bonus curve is a proposal only.
- No itch.io zip was made. Nothing was uploaded.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; Blender 5.1 (rig cleanup).
