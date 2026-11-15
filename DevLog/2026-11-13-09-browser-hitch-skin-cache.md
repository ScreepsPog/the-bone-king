# The browser hitch: found and fixed (2026-11-13 17:35 PST to 18:45 PST)

## When

2026-11-13 17:35 PST to 18:45 PST.

## Asked

The director wrote: "It is time to try to optimize. It lags a lot when new models either spawn in or walk into draw
distance. I want you to investigate and find ways to fix this." And during the testing: "I was seeing the hitching on my
side during your testing so it is something else going on".

## What the agent did

Claude Code (Claude Fable 5.2).

- **Measured before changing anything.** On the desktop, creating a figure costs 0.08 ms and the whole simulation
  0.14 ms a frame: no hitch there. In the browser the game dropped 2 to 5 frames about every 1.4 seconds even standing
  still, and stopped doing so when paused.
- **Ruled out the agent's own guesses.** Pose switching, hiding and showing figures, and the simulation's timers were
  each switched off or timed in turn; the hitch stayed.
- **Found the cause.** Counting the browser's graphics calls showed 176 new buffers and 137 new bind groups every
  frame with 137 models drawn, and none when paused. They came from the engine's GPU skin cache, which the project
  template switches on: in the browser it runs a separate pass for every animated model every frame. The browser frees
  those buffers in bulk, and that is the dropped frames.
- **Fixed it.** The skin cache is off in `Config/DefaultEngine.ini`. Models are animated the ordinary way.
- **Result in the browser pane:** 0.1 new buffers a frame (was 176), 4.2 ms a frame (was 7.2), and no dropped frame in
  10 seconds while 60 skeletons and 80 grunts were spawned beside the wizard.
- **Kept for next time:** `Wiz.Perf`, `Wiz.PerfTest`, a frame-time line in the debug overlay, and test keys that work
  while the overlay (H) is shown: J models on/off, K 40 grunts, L 30 skeletons, M test mode.

## Tests

- New unit test `WebTesting.Model.TheSkinCacheIsOff`; it failed with the old setting. 140 pass.
- New scenario `perf_models`. 16 scenarios pass. `day_six_orders` failed once in four runs: the hunters killed the
  archer before the scenario looked. It is a timing weakness of that scenario, left as it is.

## Build

Shipping web build, `BUILD SUCCESSFUL`, 259 MB, served at http://localhost:8900/. No itch.io zip.

## Not done, not checked

- Not checked at 4K or in Brave, where the director plays.
- With models off, about 40 small buffers a frame were still created before the fix; after it the count is 0.1 with
  models on, so that was the wizard and the same cause.
- Figures change the pose they copy about 600 times a second in a large fight. It costs little; not reduced.
- The first sight of a new kind of model may still compile shaders once. Not seen in this session.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; the Claude desktop app's
browser pane for the measurements.
