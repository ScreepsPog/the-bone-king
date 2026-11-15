# Research, half-speed shrine, garrisons, and the Outskirts block-out (2026-11-12 19:50 PST to 20:30 PST)

## When

2026-11-12 19:50 PST to 20:30 PST. (The round was asked for earlier in the evening and interrupted by a usage limit.)

## Asked

- "As for upgrades at the shrine, it should be one time research for better armor, better weapons, and better
  stats. I want armor, health and base damage to be separated from research upgrades. We can make this another tab
  later."
- "instead of continual play while in the shrine, lets make it to where the game slows down to .5."
- "I also don't want the hud to say it's unavailable, if it's unavailable, just don't show it."
- Lighting: "I will probably draw the levels in 2d, have you make a block out in unreal, then I'll go in and add
  meshes to make the level."
- "I like all the proposal you gave me, let's implement them and see how they work." (Posts, waking, the commander,
  patrols.)
- A drawing of the first level: "H/T/F/PS=House, RIP/R=Graveyard, HOS/+=Medical, Station=Boat Dock... lamp posts
  that have yellow on them. Blue is the river, brown is the walk ways, and green is a big farmland. Use basic
  shapes and materials to block this out."
- A separate thread for questions about meshes, materials and the outside services.

## What the agents did

Claude Code (Claude Fable 5.2) wrote the tests first and built it. A second session (Claude Opus 5.5) was started
as the advice thread for assets and services; it read the design document and is waiting for questions.

- **Research.** The shrine's upgrades are bought once: Spear of Sacrifice (12 bones), Bone armor (15, skeletons +15
  health), Bone blades (15, skeletons +4 damage a second), Dark focus (15, Bolt +5 damage). A finished research
  shows "Researched" and loses its button. Guards are still bought in bulk (x1, x5, Max). Stepwise armor, health
  and base damage are left for a second tab.
- **Half-speed time.** While the shrine's menu is open the whole game runs at half speed (`ShrineTimeScale`).
- **HUD.** The spell's line shows only when a cast would raise something. The spear's line shows only once it is
  researched and the army can pay for it.
- **Posts.** Enemies stand at posts in groups. A group charges when the wizard or a skeleton comes within 900 units
  of one of them, or when one of them is hit.
- **Commander.** 400 health, 25 damage a second, 15 bones, a large black and gold block. Killing him takes the area:
  the HUD says "The commander has fallen" and the patrols stop.
- **Patrols.** Every 35 seconds, while the commander lives, 5 grunts come from the next manned post.
- **The Outskirts.** The drawing is blocked out at 10 units a pixel: the river (water stops feet, Bolts fly over),
  two bridges, the walkways, the farmland, five graveyards, about 120 houses, seven medical buildings, the boat
  dock, two coal sheds, 49 trees, 20 lamp posts, walls around the edge. 11 posts (90 enemies) and the commander at
  the top right. The wizard starts at the bottom of the east road; the shrine stands where the middle bridge meets
  it. The game now starts in this level; the old test bed is `Wiz.Map Practice`.
- **For testing.** `Wiz.Map [Outskirts|Practice]`, `Wiz.Garrison <Grunts> [Brutes] [Distance]`,
  `Wiz.Commander [Distance] [EscortGrunts] [EscortBrutes]`, `Wiz.WakeAll`, `Wiz.Patrols [0|1|now]`,
  `Wiz.Buy <Spear|Armor|Blades|Focus|Guard> [Count|Max]`.

## How the drawing was read, and what was guessed

- Houses: every H, T, F, PS, Th, S, P, J, $ and unlabelled building. Medical: Hos and +. Dock: Station.
- "Coal" is two small black sheds. The pond beside the river is water.
- The light blue lines beside the east road were read as part of that walkway, not as a second stream.
- Positions and sizes were read by eye and are approximate. Three houses were nudged a few pixels off the walkways.
- The way through (bottom of the east road to the top right) is my choice. The drawing does not say where the
  wizard enters or where the walls are.
- Lamp posts glow but give no light yet.

## Tests

- 12 new tests. 10 failed against stubs first (posts, commander, water, research, shrine time, menu rows); the
  three layout tests were written with the layout, so they had no old code to fail on.
- **Tests changed because the direction changed:** the five-level armor test was replaced by
  `Research.EachIsBoughtOnce`; the menu button test now expects one button for research rows and three for the
  Guard. 81 pass.
- The six older key-press scenarios now begin with `Wiz.Map Practice`, since the game starts in the Outskirts.

## What failed and the fix

- The commander's post was inside a house's margin: the layout test caught it, and the post moved down the road.
- Without walls at the edges, the walk test showed the river could be walked around at its ends. Edge walls added.
- Frame times measured on the desktop were about 58 ms, GPU-bound. The director was running another game on the
  machine at the time, so no frame rate from this round is valid, and none is reported.

## Web package and browser check

Development build, `BUILD SUCCESSFUL`. In the browser pane the game starts in the Outskirts: the east road, the
river, houses, a tree and a lamp post are drawn, Bolt fires, and the HUD shows 90 enemies at their posts. That is
all that was checked. The frame rate shown there (and on the desktop) was taken while another game was running on
the machine and is not valid; it must be measured again on a free machine before anything is concluded.

Screenshots: `Images/2026-11-12-09-outskirts-start.png`, `-outskirts-top.png`, `-outskirts-first-post.png` (the
first post charging), `-research-menu.png`, `-browser-outskirts.jpg`.

## Not done

- Not play-tested by a person. Post sizes, the wake distance and the patrol timing are first guesses.
- A patrol can appear on the far bank of the river, where it cannot reach the wizard.
- Taking the area only shows the message. There is no gate, next level, or final cut screen yet.
- The stats tab, mobile controls and the lighting pass are not started.

## Tools used

Claude Code (Claude Fable 5.2, and Claude Opus 5.5 for the assets advice thread) with the unreal-toolkit plugin;
Unreal Engine 5.9 Preview; the Claude desktop app's browser pane.
