# What was used

Credits for the 2026 Epic AI MegaJam entry. The jam began 2026-11-11, 9 PM PST.

## AI tools

- **Claude** (Pro/Max plan), in Claude Code with Claude Fable 5.2 and Claude Opus 5.5: all code, design document,
  tests, builds, dev logs, and the HUD and menu designs (drawn as Claude artifacts).
- **Claude Toolkit** (unreal-toolkit), a Claude Code plugin made by the director and Claude before the jam: lets
  Claude build the project, run tests and editor scripts, and drive the Unreal editor. It ships no game content.
- **superpowers**, a Claude Code plugin for planning and testing workflows.
- **Unreal's AI toolset plugins** (editor only): how Claude works inside the editor.
- **Tripo**: every 3D model made for the game. Characters: Wizard King, enemy soldier, Brute, Archer, Assassin,
  commander, Marksman, skeleton army and Guard. Buildings: Peasant Cottage, Burned Cottage, Barn, Two-story House,
  Shrine, kingdom wall, gate, gate door and tower. Props and weapons: lamp post, bridge, bone pile, arrows, spears,
  swords, bows, hammer and dagger. Their textures came from Tripo too.
- **fal.ai** (Nano Banana 2): concept art for the Tripo models, the HUD and shop icons, the shrine pictures and the
  start screen picture.

## Engine and tools

- **Unreal Engine 5.9 Preview**, packaged for the web with **emscripten 6.0.10**.
- **Blender 5.1**: fixing the rigs and scale of the Tripo models.
- **git with LFS**: version control.
- **Claude Test Helper** plugin: helpers for automated tests.

## Art from other sources

- **Fab**: "FREE Stylized Forest Sample | Stylized Trees & Foliage Pack" by StyleHex Studio, for all the trees.
  https://www.fab.com/listings/3c1a31b6-e523-4a0f-97d0-26b0db69b6bc
- **Megascans**: two ground textures, Dirty Soil Ground and Wild Grass.
- **Unreal Engine content**: the mannequin skeleton every character uses, the Roboto font, and the sky, cloud and
  lighting assets.
- **Cinzel Bold** font (SIL Open Font License 1.1, The Cinzel Project Authors): titles and labels.
  https://github.com/NDISCOVER/Cinzel

## Animation

- **Mixamo**: archer shoot, aim and draw arrow; brute hammer slam and hammer down; wizard Bolt.
- **Paragon** (Epic's free Paragon assets): deaths, stun, sword blows, and the brute, assassin and commander attacks.
- **Unreal mannequin animations**: walking and running.
- Attack montages put together by the director in the editor.

## Audio

- **WeLoveIndies**: all music and sound effects in the game.

## Other

- **itch.io**: hosting the web build.

## Tried, not used

Hyper3D Rodin (results not liked), PBRgen (might still be used), Cursor and Claude API credits (unlikely).

## Made before the jam

- The Claude Toolkit plugin.
- Project setup only (Nov 10 to 11): an empty C++ project, git, web build settings, renderer settings, a package
  check script and test rehearsals. No game mechanic, level or asset existed before the jam.
- emscripten and the superpowers plugin were installed.
