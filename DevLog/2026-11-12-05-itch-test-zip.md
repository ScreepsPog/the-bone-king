# Test zip for itch.io (2026-11-12 12:52 PST)

## When

2026-11-12, finished 12:52 PST.

## Asked

"Create a version I can upload to itch.io to test through their platform before continuing."

## What the agents did

Claude Code (Claude Fable 5.2) packaged the Shipping web build, made the zip and checked both. Nothing was uploaded:
the director uploads.

- Shipping web package: `BUILD SUCCESSFUL`. The page carries the real-pixel setting (2.0) and no Chrome banner.
- Zip: `Saved/JamZips/Screeps_Day2_Web.zip`, 76.7 MB, 32 files, 176.6 MB unzipped, largest file the `.wasm` at
  133.0 MB. `index.html` is at the top. Within itch.io's limits (500 MB total, 200 MB a file).
- New script `Tools/Jam/make_web_zip.py <stage> <zip>`: asks the engine's deploy planner which files belong in a
  zip upload and zips exactly those. It replaces the hand steps of the rehearsal.
- Local boot of the Shipping build in the browser pane: ready in 4.5 s, the game runs (wizard, grunts, walls,
  "Bones: 0 / 3", Overwhelm filling). The debug panel and the debug keys are not in a Shipping build, by design.

## Not checked

The zip on itch.io itself. That is the point of the upload.

## Tools used

Claude Code with Claude Fable 5.2, Unreal Engine 5.9 Preview.
