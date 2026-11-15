# Level pieces as actors, mesh houses, and a landscape ground (2026-11-14 00:55 PST to 01:46 PST)

## When

2026-11-14 00:55 PST to 01:46 PST.

## Asked

The director wrote:

- "Go into UE and make everything you have in the Outskirts Layout (crowd renderer) as actual blocks that can be moved
  or deleted. Also make the ground a landscape that can be painted over using textures".
- "additionally, replace some of those block houses with the mesh houses. Not all of them but some to get a general
  feel".

## What the agent did

Claude Code (Claude Fable 5.2).

- **Pieces.** A new actor, Level Piece, stands for one house box, tree, stretch of river, road, field or lamp. The
  layout actor has a button, "Place Pieces As Actors", that puts every piece of the code-made layout into the level
  once. 272 were placed. When a run starts the game reads them from the level, so moving or deleting one changes what
  blocks and what is drawn. The game still draws them in batches, as before, so their number costs the web build
  nothing; the shapes seen in the editor are not in the packaged game.
- **Mesh houses.** 16 house boxes were swapped for the four house meshes, where a mesh fit clear of roads, river,
  other buildings, posts and patrol roads. 95 boxes remain. The meshes are much larger than most boxes (about 11 by
  10 meters against 6.6 by 4.5), which is why only some fit.
- **Landscape.** The template's floor is gone. "Ground Landscape" is 315 by 252 meters with four paint layers
  (Ground, Grass, Dirt, Stone), each a texture and a tint on `Content/Landscape/MI_Ground`.
- **Lose sight** (the round before, written but not built): built and tested here. All five of its tests pass, and
  no older test failed.

## Found on the way

- The landscape was black in the game at first. The headless script run has no renderer, and the engine merges paint
  layers with the renderer. Running the same script with one fixed it. The script now checks the merged result and
  fails loudly when there is none.

## Tests

- New unit test: a turned piece blocks the square box around it. The level test now also asks for the piece actors,
  the layout actor's switch and one landscape; it failed before the level was made, as it should.
- **A test was corrected**: the agent first wrote "at least 300 piece actors" before counting them. There are 256
  after the 16 swaps, so the test asks for at least 200.
- **A scenario was changed**: `day_eleven_level` expected 0 placed buildings; it now expects 16, and 256 placed pieces.
- 153 unit tests pass.
- Scenarios: `day_three_outskirts` and `day_two_walls` failed at first: the practice layout, loaded in the same level, picked up the 16 mesh houses as walls. Placed buildings now count only for the Outskirts. All 18 pass.
- Screenshot looked at: `Images/2026-11-14-03-level-landscape.png` (ground lit and shadowed as before, pieces drawn).

## Build

Shipping web package: BUILD SUCCESSFUL, 240 MB staged (227 before). Opened in the browser pane at `http://localhost:8900/`: the landscape ground, its shadows and the pieces draw. No zip made.

## Not done, not checked

- Nobody has opened the level in the editor: it was made by script. Painting the landscape was not tried by hand.
- No mesh house was looked at in a screenshot; their doors face no particular way.
- The start, shrine, posts, commander and patrol roads are still code and cannot be moved in the editor.
- The landscape must stay flat. Sculpting it is not supported by the game.
- The landscape layers have no normal maps.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview (headless Python, its
landscape editor subsystem).
