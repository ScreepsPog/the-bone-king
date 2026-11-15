# Bones, even groups and health bars (2026-11-12 12:15 PST to 12:45 PST)

## When

2026-11-12 12:15 PST to 12:45 PST.

## Asked

The director sent a picture of about 25 skeletons packed into one block and wrote:

- "I don't like this type of clumping up, or anything that is more than 15 together. There should typically be 3-6
  groups, depending on the overall size of the army."
- On how the army is made: "I like 1 with 3 and 4's idea. The general idea is there is multiple enemy ai types and
  multiple friendly army ai types. For 1, let's make it to where the player must actually walk over/near it to
  collect it, increasing risk." (1: kills drop bones and enough bones assemble a skeleton. 3: bones also pay for
  abilities. 4: tougher enemies give better skeletons.)
- "Add a little bar above enemies WHEN they take damage to show their health. Try to make this as performative as
  possible. It should never be up all the time."

## What the agents did

Claude Code (Claude Fable 5.2) wrote the tests first, built the three features, packaged the web build and checked
it in the browser pane. Commit `1c26193`.

- **Groups.** The army stands in 3 to 6 groups spread evenly around the wizard: one group for every 7 skeletons
  (3 groups up to 21, 4 up to 28, 5 up to 35, 6 at the cap of 40). Skeletons are shared out evenly, so the largest
  group at a full army is 7, far under the limit of 15. A skeleton goes back to its group after a fight. This
  replaces the rule from an hour earlier ("walk to the nearest skeleton"), which is what let one block grow to 25.
- **Bones.** A dead enemy leaves a bone where it fell. The wizard collects bones within 130 units of him (the white
  circle in the debug overlay). Bones nobody collects fade after 30 seconds. Every 3 bones assemble one skeleton
  beside him. With a full army the bones are kept. A kill no longer raises a skeleton by itself.
- **Health bars.** An enemy has a bar only for 2.5 seconds after it takes damage, fading over the last half second.
  Each bar is two flat rectangles drawn by the HUD; an enemy that was not hit costs nothing.
- **For testing.** `Wiz.AddBones <Count>`, `Wiz.HealthBars [0|1]`, and every number through `Wiz.Tune`
  (`BonesPerSkeleton`, `BonePickupRadius`, `BoneLife`, `GroupTargetSize`, `GroupDistance`, `HealthBarSeconds`...).
  The HUD shows "Bones: 2 / 3" under the army count.

Not built this round, by design: bones paying for abilities (comes with the bone spear) and more kinds of enemy and
skeleton (comes with the Brute).

Tests: 9 new ones and 3 changed ones failed against the old code first. Three older tests were changed because the
direction changed (they asserted that a kill raises a skeleton); they now assert that it leaves a bone. 46 tests
pass, plus three key-press scenarios (`day_two_bones` is new).

## What failed and the fix

- **The first sim script stopped half way** on a wrong indentation in a search string, after it had already changed
  a header, which broke the build. Fixed the string and ran it again.
- **Two more shell scripts with apostrophes failed to parse**, as noted before. They go through script files.

## Screenshots

- `Images/2026-11-12-bones-groups-health-bars.png`: desktop, health bars over grunts that were hit, groups fighting,
  "Bones: 0 / 3".
- `Images/2026-11-12-browser-bones.jpg`: the web build, bones lying where grunts fell.

## Web package

Development build, packaged 12:38 PST, `BUILD SUCCESSFUL`, 291.0 MB staged, `.wasm` 192.0 MB.

Browser pane check, 66 s of moving and firing: 119 to 120 fps, 89 kills, 35 bones collected or lying about, army 9
(it was 40 at this point before bones), 3 groups, health bars on hit grunts. One old console line remains ("an
unknown error occurred when fetching the script"); the game is not affected.

## Open

- The bone numbers are first guesses. With a script walking in a square, most bones were left behind.
- With 3 skeletons to a group, a group reads as three loose skeletons, not a wall.

## Tools used

Claude Code with Claude Fable 5.2, Unreal Engine 5.9 Preview, the Claude app's browser pane.

## Time spent

About 30 minutes.
