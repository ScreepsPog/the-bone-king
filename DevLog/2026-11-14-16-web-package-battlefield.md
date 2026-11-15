# Web package with the battlefield edge, Marksman changes and the missing animations and weapons (2026-11-14 20:39 PST)

## When

2026-11-14 20:28 PST to 20:39 PST.

## Asked

The director's standing request from the round before: "Then build a playable web version, no a itch package yet".
This package carries the fix for what the director saw in the last one (archers and Marksmen with no bow, using the
unarmed attack).

## What the agent did

Claude Code (Claude Fable 5.2).

- Packaged the Shipping web build of commit `af7c0ab` (BuildCookRun, Wasm) into `Saved/WebBuild/Wasm`. It is served
  at http://localhost:8900/. No itch.io zip.
- This is the first package whose cook list includes `/Game/Animations`, `/Game/Weapons` and `/Game/Props`.

## Tests

- Before packaging: 195 unit tests passed, 0 failed; 27 of 28 scenarios passed (`day_two_walls` stops at the barn
  the director moved).
- The package was **not opened or played** by the agent this time: the director asked to hold further work until
  after compacting.

## Build

Shipping web package: succeeded.

## Not done, not checked

- Not seen in a browser: whether the bows, the shooting animation, the hammer, the arrow, spear and bone pile
  models now show there. The cause was found and fixed, but only playing the package proves it.
- Held at the director's word, for after compacting: the settings menu (the UI thread's `settings-handoff.md` is
  ready) and importing the four Kingdom models (wall, gate, gate door half, tower).
- The Paragon packs were not imported: that needs the director's own Fab sign-in.
- No itch.io zip; nothing uploaded.

## Tools used

Claude Code (Claude Fable 5.2); Unreal Engine 5.9 Preview (UAT BuildCookRun, Wasm).
