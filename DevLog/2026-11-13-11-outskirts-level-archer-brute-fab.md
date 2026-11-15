# The Outskirts as a real level, archer and brute models, sharper buildings, Fab library (2026-11-13 22:50 PST to 23:30 PST)

## When

2026-11-13 22:50 PST to 23:30 PST.

## Asked

The director wrote:

- "Use the toolset inside of ue editor to see fab inside the editor. This should not require computer use or signing
  in. Additionally, any assets you see that can be used int his project and make sense, see if it allows usage with ai
  and if it doesn't, what limitations does it have."
- "Create the level we have been playing as a actual level so that I can edit it and add buildings. If it's already a
  level, I am not seeing it under the level filter when opening."
- "for all the buildings, find a way to enhance the quality of the textures on it ... It almost feels blurry. This
  might be due to it being 1024x1024."
- "I redownloaded the archer model with the rig." And: "add the brute to model to the enemy ai".
- "If there is anything new in the toolset that the unreal-toolkit doesn't have, send a message over to that thread to
  add it".

## What the agent did

Claude Code (Claude Fable 5.2).

- **Fab.** The editor's own Fab toolset read the library (239 items) with the director's sign-in already in the
  editor. Fourteen packs that fit a dark medieval outskirts were looked up for their license and their "NoAI" tag.
  Nothing was installed or added.
- **Level.** There was no level before: the game ran on the engine's empty template and built everything in code.
  `Content/Maps/Outskirts` now exists and is the startup map. One actor in it, "Outskirts Layout", draws the layout
  in the editor. Buildings dropped into the level block units, Bolts and the wizard. A switch removes the house boxes.
- **Textures.** Building color and normal maps keep their full 2048; the color map's smaller copies are sharpened.
- **Models.** The archer (165) and the brute (240) went through the rig fix and are drawn in the game.
- **Toolkit.** The gaps were sent to the plugin thread, which answered that version 0.4.1 has them.

## Tests

- New unit test: a placed building blocks its footprint, and the Outskirts level is in the project. The archer and
  brute joined the model size test. 146 pass. The new test was written with the code, not run against the old code.
- New scenario `day_eleven_level`; `day_nine_models` gained an archer and a wider model count. 18 scenarios pass.
- Screenshots looked at: `Images/2026-11-13-11-models-fight.png` (the brute model beside the wizard),
  `Images/2026-11-13-11-level-boxes.png`.

## Build

Shipping web build on `-map=/Game/Maps/Outskirts`, `BUILD SUCCESSFUL`, 227 MB staged (225 before), served at http://localhost:8900/. It boots and shows the Outskirts in the browser pane. No itch.io zip.

## Not done, not checked

- No building is placed in the level: that is the director's to do. Nobody has opened the level in the editor yet to
  see the layout picture there; it was made and saved headless.
- The layout (roads, river, trees, posts) cannot be moved in the editor. Only buildings can be added.
- Placed buildings are not cut away around the cursor.
- The brute and the archer have no attack animation.
- Polygon counts and texture sizes of the Fab packs: the toolset does not report them.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview and its editor tool server
(Fab toolset); Blender 5.1 (rig cleanup). The archer and brute meshes and textures came from the partner mesh service
(Tripo), made by the director.
