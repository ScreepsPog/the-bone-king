# Bolt range, a kill wakes the group, life at posts, road patrols, larger scale (2026-11-13 00:00 PST to 01:10 PST)

## When

2026-11-13 00:00 PST to 01:10 PST (the start is approximate; the round was interrupted once by a compaction).

## Asked

After playing the Outskirts, the director wrote:

- "I found a bug where if you fire a bolt at a small guy and one shot them, it doesn't alert the rest."
- "I also want the AI to feel alive and move around a little bit, even when they're at a station/checkpoint spot.
  Some can be idle, but there should be general life to the game."
- "Add patrols to the roads that are more frequently walking around the streets so that a player camping or afk is
  not safe."
- "Add more enemy AI in general so upgrades have magnitude and are more obtainable."
- "Also change the player's bolt to have a max distance. Currently, they can snipe/kite from any enemy and never be
  in danger."
- "the scale of the map is slightly too small, in terms of the building sizes. While the player is a god, they are
  not the size of a house."
- "I am thinking of making the camera be able to moved during angled mode. I like the concept of 3D mode at this
  stage... though I'll wait until everything is to scale like I want it." (Noted in the design document; not built.)
- "You are free to do tests now." (Frame rate.)
- "When you finish the current task/plan, create a build I can upload to itch for testing."

## What the agent did

Claude Code (Claude Fable 5.2) wrote the tests first, then the code.

- **The bug.** An enemy killed in one hit was removed before its group was checked for "has anyone been hurt", so
  the rest stood still. Now a death wakes the dead enemy's group.
- **Bolt range.** A Bolt flies 850 units (`BoltRange`), less than the 900 at which a post notices the wizard. It was
  3600 before (1.5 seconds at 2400 a second).
- **Life at a post.** About two in three of a group stroll within 220 units of where they were posted and stop a few
  seconds between walks; the rest stand. The commander stands. (`PostWanderRadius`, `PostWanderSpeed`,
  `PostIdleShare`.)
- **Road patrols.** Four roads of the Outskirts each have a group of 5 grunts walking end to end and back at 150 units
  a second. They notice the wizard like a post does. A dead patrol is replaced after 40 seconds at the end of its road
  farther from the wizard, never within 1800 units of him, while the commander lives. The hunting parties that come
  straight for the wizard now come every 25 seconds instead of 35.
- **More enemies.** 14 posts (11 before) with larger groups: 167 enemies at posts and in the escort (98 before), 20
  on the roads. The cap went from 120 to 320.
- **Scale.** One pixel of the drawing is 15 units instead of 10, so the whole level is 1.5 times larger beside the
  wizard. Houses are 420 tall (260 before); the other buildings, trees and lamp posts were raised to match. Trees and
  lamp posts keep their thickness. `Wiz.MapScale [8-22]` changes the scale while playing.
- **For testing.** `Wiz.MapScale`, and `Wiz.Tune` for `BoltRange`, `PostWanderRadius`, `PostWanderSpeed`,
  `PostIdleShare`, `PatrolWalkSpeed`, `RoadPatrolSize`, `RoadPatrolRespawn`. `Wiz.State` now has `roadPatrols` and
  `mapScale`. `Wiz.Patrols 0` also stops patrols being replaced.

## Tests

- 6 new tests, all failed against stubs first, all pass now. 87 pass in total.
- **Tests changed because the direction changed:**
  - `Sim.EnemyCap` expected 120; it now reads the cap and expects 320.
  - `Garrison.StandsUntilTheWizardComes` expected the grunt not to move at all; it now expects it to stay within
    the wander distance of its post.
  - `Garrison.WakesWhenHit` fires a Bolt from 2000 away; it lifts the Bolt's range for that test.
- 7 scenarios pass. `day_three_outskirts` also checks the scale and that four roads are patrolled.

## Build for itch.io

- Shipping web package: `BUILD SUCCESSFUL`.
- Zip: `Saved/WebBuild/WizardJam-web-2026-11-13.zip`, 76.7 MB (176.6 MB unzipped, largest file 133.1 MB). The size
  check passes (limits 500 MB total, 200 MB a file).
- Loaded in the desktop app's browser pane from a local server: the Outskirts appear at the new scale, the wizard
  walks, enemies stand at the first post and a group is on the road.

## Frame rate

- Desktop development build, 1280x720, 188 enemies: 2.2 ms a frame.
- Browser pane, Shipping build, 1024x768: the usual gap between frames was 25 ms (about 40 a second), with gaps of a
  full second mixed in. Gaps of exactly one second are what a browser does to a page it treats as in the background,
  so the pane is not a fair place to measure. No conclusion is drawn; the director's own browser is the real test.

## Not done, not checked

- Not play-tested by a person. Bolt range, wander distance, patrol speed and timings are first guesses.
- A hunting party can still appear on the far bank of the river.
- The camera that can be moved and the 3D view: waiting for the director's word on scale.
- Nothing was uploaded; the director uploads to itch.io.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview; the Claude desktop app's
browser pane.

Screenshots: `Images/2026-11-13-01-outskirts-start.png`, `-outskirts-top.png`, `-outskirts-first_post.png`.
