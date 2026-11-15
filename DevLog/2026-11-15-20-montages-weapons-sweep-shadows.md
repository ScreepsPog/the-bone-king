# The director's montages, weapons in hand, the brute's sweep, shadows and the thud's timing (2026-11-15 00:20 PST to 00:53 PST)

## When

2026-11-15 00:20 PST to 00:53 PST.

## Asked

The director reported shadows going away as the cursor moves ("This has to do with the screen through area of the
cursor"); made montages for every chosen animation ("the same animation name, with Anim_ added to the beginning",
"a PlaySound Notify in most of them"); asked that the deaths be gone through first so "the sound for hitting the
ground lines up perfectly on when the notify begins ... when the actual sound is heard"; renamed `Hammer_Socket` to
`Weapon_Socket` and added `Dagger_Socket`, `Sword_Socket`, `Commander_Spear_Socket` and `Commander_Attack_Sword`;
asked for the brute to keep its area attack and gain "a new basic attack that uses
Anim_A_Terra_Primary_Melee_D_slow and does a sweep attack in a shape of about 1/3 sphere"; and said the volume
controls "did not seem" to work as intended. "Do not build yet" (taken as: no web package).

## What the agent did

Claude Code (Claude Fable 5.2).

- **Shadows.** Cause: the see-through circle is cut out of walls, roofs and leaves by their opacity mask, measured
  on the screen. Shadows are drawn from the light's side, where "the screen" is the light's, so the same circle cut
  holes in shadows in the wrong places. `Tools/Art/see_through_shadows.py` put a Shadow Pass Switch in the seven
  materials: shadows now use the mask without the circle (leaves keep their own cut-out). **The agent's two test
  screenshots showed no missing shadow before or after**, so the fault itself was not reproduced; the fix is from
  the cause, not from a seen before-and-after.
- **Montages.** Read all fifteen. Their notifies are named markers (`Attack`, `ThrowSpear`, `PlaySound`, and `Fire`
  on the three older ones), not sound-playing notifies, so nothing plays twice. The game now takes `Anim_<Name>`
  wherever it took the animation, and times each blow, the throw and each body fall from them.
- **Deaths first.** Measured each body-fall file for when its thud is first heard (dirt 0.545 and 0.54 s in, grass
  0.225 and 0.195 s). The sound is now started that much before the death's `PlaySound` moment, so the thud lands on
  it; nothing is cut from the file unless the moment is nearer than the lead-in. The ground kind and which of the
  two sounds are picked at the death.
- **Weapons.** In hand on the new sockets (see the design document). The commander's spear shows only while he
  draws back. `Commander_Attack_Sword` is not on the model (the others are), so his sword uses `Weapon_Socket`.
- **Brute sweep.** New in the simulation: a third of a circle across its front, between slams (numbers in the
  design document; all on `Wiz.Tune`).
- **Volume.** Read the engine's code for the path the sliders use and found no fault in it; could not listen. One
  real editor problem found and fixed: the game set the editor's own "quiet when unfocused" value, which stays set
  after play stops. What exactly misbehaved is asked of the director.

## Tests

- New unit tests (3, `Tests/MontageSweepTests.cpp`): the montages are what is loaded and their notifies give the
  times; the thud's lead; the sweep's shape, its place between slams, what it hits, and slams-only with
  `BruteSlamEvery 0`. One line of the new sweep test was corrected (it forgot the army is hurt more away from the
  wizard).
- **Tests changed**: the hidden swing figures are 78 now (the brute has a second attack): `day_nine_models`,
  `day_sixteen_sound_deaths`.
- Unit tests: 219 passed, 0 failed.
- Scenarios: 31 of 32 pass. New: `day_seventeen_montages_sweep`. `day_two_walls` still stops at the barn the director moved.
- Screenshots looked at (`DevLog/Images/2026-11-15-20-*.png`): skeletons with swords and the brute mid-sweep; the
  commander with his spear raised and the throw's lane.

## Build

Editor module compiled. **No web package** (as told). This and the last round's code are still not compiled for
the web.

## Not done, not checked

- **Nothing was heard**: the thud's timing is arithmetic on measured files and the montage's times.
- The shadow fault was not reproduced by the agent.
- How each weapon sits in the hand (angle, grip) was seen in one frame only; the swords looked held, not checked
  closely.
- The sweep's numbers are a first guess. Its `PlaySound` and those of the other attack montages play nothing yet:
  to be gone through with the director.
- The sweep was not checked against the montage beyond its `Attack` time (0.369 s).

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview.
