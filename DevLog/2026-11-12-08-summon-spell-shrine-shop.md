# Summon the Afterlife, the shrine shop, and the new direction (2026-11-12 15:45 PST to 16:07 PST)

## When

2026-11-12 15:45 PST to 16:07 PST.

## Asked

- "Make the shrine's open distance bigger. Perhaps .5x more."
- "I like the idea of upgrading at the shrine, whether that be unlocking, increasing stats or adding abilities."
- "By default when you start the game, you collect bones and will have a basic spell that is called 'Summon the
  Afterlife' (subject to change) that then summons the friendly skeleton army, whether that only does 1 or uses all
  the bones is something I will need to test/think on."
- "The player will not have spear of sacrifice by default, this will be purchasable."
- "small guys spawn on cast, while bigger/better guys must be bought. We need to make sure it can be bought in bulk
  so you don't have to spam click the menu."
- Mobile friendly "if time permits, so UI controls."
- Polish: "more like a game rather than a mobile game... using lighting to it's maximum that is allowed, shadows,
  baked lighting, etc. What tools can we use that can achieve this?"
- Direction: "the player charging into his old kingdom to take it back... the enemies shouldn't chase the player by
  default, but rather have the player move throughout the maps/levels to slowly progress." Three areas: outskirts
  and townhouses; inside the kingdom walls with houses, blacksmiths, armorers; the castle, to the throne room.

## What the agents did

Claude Code (Claude Fable 5.2) wrote the tests first, built it, packaged the web build and checked it in the browser
pane. Commit `f9f1496`.

- **Summon the Afterlife** (Q or Space). Bones no longer turn into skeletons by themselves. The wizard holds them
  and the spell spends them: 3 bones a plain skeleton, standing up beside him, up to the army cap. One cast uses all
  the bones it can. `Wiz.Tune SummonMax 1` makes it one skeleton a cast, so both can be tried. The HUD says how many
  a cast would raise.
- **The spear is locked.** The HUD says "locked (buy it at a shrine)". It costs 12 bones, once.
- **The shrine is a shop.** Three rows: Spear of Sacrifice (12 bones, once), Guard skeleton (8 bones each), Bone
  armor (10 bones a level, 5 levels, +10 health for every plain skeleton, including those already in the army).
  Guards and armor have x1, x5 and Max buttons. A row that can buy nothing is gray. Every price is a `Wiz.Tune` name.
- **The shrine's circle is 345 units** (was 230).
- **For testing.** `Wiz.Summon`, `Wiz.Buy <Spear|Guard|Armor> [Count|Max]`, `Wiz.UnlockSpear [0|1]`. `Wiz.State` now
  has `spearUnlocked`, `armor` and `summon`.
- **Design document.** The new direction is written in `docs/GDD.md`: the kingdom, the three areas (the Outskirts,
  Inside the Walls, the Castle), enemies that hold their ground, the shop, the lighting plan, phones if time permits.

## Tests

- 8 new tests; 10 tests failed against the old code first (the 8, and 2 existing ones rewritten for the new rule).
  69 pass now.
- **Tests changed because the direction changed**, said plainly: `Bones.ThreeAssembleASkeleton` became
  `Bones.ThreeMakeASkeletonWhenSummoned`; `Bones.KeptWhenArmyIsFull` and `Bones.BruteLeavesThree` now cast the spell;
  the spear tests unlock the spear first. None was weakened: each still checks the same numbers.
- Six key-press scenarios pass. `day_two_shrine` now buys the spear, three Guards with Max, and two levels of armor.

## Web package and browser check

Development build, `BUILD SUCCESSFUL`. Browser pane, 240 to 520 fps:

- Bones were held (9, army 0) and the HUD offered "raises 3"; Q raised 4 skeletons from 12 bones.
- The shrine menu opened in its circle with the shop rows and the bone count.
- A real click on Guard x1 bought a Guard (army 1).
- **Not confirmed by a click in the browser:** the spear's Buy button and x5 / Max. Two clicks sent a few
  milliseconds apart landed in one frame and did nothing; a person cannot click that fast. The same buttons are
  covered by the button test and by `Wiz.Buy` in the scenario. Please click them once by hand.
- The scripted wizard, with no army and no Bolt while shopping, was overwhelmed twice in about 10 seconds. Shopping
  under attack is dangerous, which is the intent, but it may be too harsh before the first skeletons.

## Proposal: enemies that hold their ground (not built)

- **Garrisons.** Each area is a chain of pockets (a farmyard, a street, a square). Each pocket has a set group of
  enemies standing at their posts.
- **They wake by distance or by being hit.** A group charges when the wizard or a skeleton comes within about 900
  units, or when one of them takes damage. Until then it stands, so the player picks the fights and the pace.
- **A lull is the ground already cleared.** The shrine stands in a cleared pocket; nothing comes while you shop
  unless you pulled something.
- **The area ends at its commander**, who stands at the gate with an escort. Killing him opens the next area.
- **Another view, with its trade-off:** keep a small stream of reinforcements from ahead (a patrol every 30 to 40
  seconds). Fully static enemies can make the Overwhelm meter irrelevant, because a careful player pulls one group
  at a time and is never surrounded. A patrol keeps pressure on at the cost of some of the "I set the pace" feel.

## Lighting: what the browser build can use

The web build runs Unreal's mobile renderer on WebGPU. Usable, all Unreal's own tools, no new service:

- **Baked lighting (Lightmass, CPU or GPU).** The biggest step toward "a game". It needs a real level with static
  meshes; today's walls are boxes made at run time on the engine's template map and cannot be baked.
- **One shadow-casting directional light** (moon or overcast) with cascaded shadows for the wizard and the crowd.
  The crowd's shadows are off today; switching them on has to be measured.
- **Fire:** glowing materials, bloom, and a few unshadowed point lights.
- **Reflection captures, height fog, and post-process:** bloom, color grading, vignette, tone mapping.
- **Meshes with real materials** from Tripo and PBRgen.
- Not on the web: Nanite, virtual shadow maps, ray tracing. Lumen exists only in an experimental tier that is too
  heavy for a crowd.

Each step is measured in the browser for frame rate and download size before it is kept.

## Screenshots

- `Images/2026-11-12-08-shrine-shop.png`: desktop, the shop with 40 bones.
- `Images/2026-11-12-08-shrine-shop-bought.png`: after buying the spear, three Guards and two levels of armor.
- `Images/2026-11-12-08-browser-shop.jpg`: the web build, a Guard bought by a click while grunts close in.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; the Claude desktop app's
browser pane.
