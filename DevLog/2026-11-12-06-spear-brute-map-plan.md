# Bone spear, the Brute, the Guard, and a plan for the map (2026-11-12, finished 14:59 PST)

## When

2026-11-12, afternoon, finished 14:59 PST. The work was interrupted once by a usage limit and picked up again.

## Asked

- The itch.io test upload "looks good, plays exactly the same as in browser from earlier."
- Build the bone spear. The director's picture of it, for later: "the player swipes at a group of skeletons,
  essentially grabbing them, and is turned into dust/smoke, which would screen the pop in of the bone spear."
- "As for the brute, let's see what you come up with."
- Shrines "sooner or later".
- "Plan for design of the map... mid-evil century time type setting, using a lot of darker toned objects, like
  rocks, dead or dark trees, fire, rain."

## What the agents did

Claude Code (Claude Fable 5.2) wrote the tests first, built the spear, the Brute and the Guard, packaged the web
build, checked it in the browser pane, and wrote the map plan into the design document. Commit `c2ae5c3`.

- **Bone spear** (right mouse or E). Takes the 3 skeletons nearest the wizard, flies to the point he aims at (up to
  900 units, stopping at the first wall) and explodes: 30 damage to every enemy within 260 units. A grunt dies; a
  Brute loses less than half. One second between throws. The HUD says "Spear: ready", "Spear: needs 3 skeletons" or
  the seconds left. Shown as a thin green shaft and a green disc that spreads over the area hit.
- **Brute.** 80 health (eight Bolts), slower than a grunt, more than twice the damage. A large dark block with a
  wider health bar. From 45 seconds into a run every wave brings one, plus one for each further minute, up to four.
- **Guard.** A Brute leaves one large pile of bones. Walking over it assembles a Guard at once: 60 health, almost
  twice a skeleton's damage, taller and darker. The spear takes plain skeletons before Guards.
- **For testing.** `Wiz.ThrowSpear [X Y]`, `Wiz.SpawnEnemies <Count> [Grunt|Brute]`, and the new numbers through
  `Wiz.Tune` (`SpearDamage`, `SpearRadius`, `SpearSkeletonCost`, `BruteHealth`, `GuardHealth`...).
- **Map plan.** A new section "The map" in the design document: the mood, the three zones as places along one road,
  how the layout is built from boxes first and dressed afterwards, what the browser allows (a small reused kit,
  fire as glow plus a few unshadowed lights, cheap rain near the camera, fog), and where the pieces come from.
- Shrine interact moves from E to F, since E now throws the spear.

Choices made by Claude where the director left it open: the spear costs skeletons only, not bones (a bone cost
exists as a number, set to 0); the spear is always available until shrines exist; the Guard as the Brute's reward.

Tests: 9 new ones failed against the old code first. One new test had a wrong expectation of its own (which two
skeletons are left after a throw) and was corrected; the code was right. 55 tests pass, plus four key-press
scenarios (`day_two_spear` is new).

## What failed and the fix

- **The first browser check showed 55 grunts in a block next to the wizard, no kills and almost no Overwhelm.** It
  looked like a pathing bug. It was the check: the pane was hidden when the script started, so the held mouse
  button never reached the game, and the scripted wizard, who runs twice as fast as a grunt, simply outran the
  crowd. With the pane shown and a real click first, the run was normal. It does show one true thing: a wizard who
  only runs is hard to catch.
- **The blast could not be caught in a desktop screenshot** because it lasts under half a second. The scenario
  stretches it with `Wiz.Tune BlastSeconds 3` for the picture.
- **A script stopped half way again** on a wrong indentation in a search string. Fixed and rerun.

## Screenshots

- `Images/2026-11-12-spear-blast-and-brutes.png`: desktop, the blast disc spreading, Brutes (dark blocks) coming in.
- `Images/2026-11-12-browser-brutes.jpg`: the web build, Brutes with their wider health bars, bones on the ground.

## Web package

Development build, `BUILD SUCCESSFUL`, 291.0 MB staged, `.wasm` 192.0 MB.

Browser pane check, 95 s: about 240 fps, 136 kills, 22 skeletons assembled, Brutes from 45 s on, the spear thrown
every 7 s by the script (which is why the army stayed small).

## Not checked

- The right mouse button in a real browser (the script used E). A browser may open its menu on right click.
- How the spear feels to aim by hand, and whether 3 skeletons is the right price.

## Tools used

Claude Code with Claude Fable 5.2, Unreal Engine 5.9 Preview, the Claude app's browser pane.
