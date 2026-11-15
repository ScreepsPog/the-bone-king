# Game design document

Working title: none yet (decided later). Entry for the 2026 Epic AI MegaJam, theme "Director's Cut".
Solo entry by Screeps; idea developed with a friend (credited for the idea, not a team member).

This file is the single source of direction. When a decision changes, change it here and add a line to the
decisions log at the bottom.

## Pitch

You are a godlike skeleton wizard, driven out of your kingdom by a tyrant who overwhelmed you. Charge back in and
take it: through the outskirts, inside the walls, and into the castle to the throne room. Everything you kill leaves
bones. You summon your army from them, and spend them and the army itself to grow more powerful.

**The twist** (director, relayed by the assets session, 2026-11-13 16:35 PST): the wizard is the villain. The "tyrant" is the rightful, good king who drove him
out; only the wizard calls him a tyrant. The kingdom's soldiers are its honorable defenders. The name "the Tyrant"
stays wherever the wizard speaks (the HUD, the boss's name), because that is his view.

## Theme fit

- **You direct.** You never fight alone; you direct an army you raised.
- **You cut.** Every upgrade is paid for by sacrificing skeletons. What you keep and what you cut decides the run.
- **The final cut.** Each run ends with a tally of what you raised, cut, and took.
- **The villain's cut.** The story is told the way the villain cuts it (director, relayed by the assets session, 2026-11-13 16:35 PST).

## Pillars

1. **Godlike but never safe.** You are far stronger than any enemy, and there is always a way to fall.
2. **The army is everything.** It is your shield, your damage, and your money.
3. **Every cut hurts a little.** Sacrifices must be worth it and must cost real safety.
4. **Runs in a browser tab.** Simple shapes, low draw counts, short sessions.

## Core loop

Direction of 2026-11-12 16:07 PST: the wizard is the attacker. He moves through the kingdom; the tyrant's forces hold it.

1. Push forward into ground the tyrant's forces hold, and kill what stands there.
2. Walk over the bones the dead leave, and cast **Summon the Afterlife** to raise skeletons from them.
3. At a shrine, spend bones on unlocks, better units and upgrades. Some abilities then cost part of the army.
4. Push on to the next part of the area. Each area ends with its commander.
5. Reach the throne room and bring down the tyrant.

## Player

- **Move:** WASD. **Aim:** mouse. Gamepad later if time allows.
- **Bolt** (left mouse): fast ranged attack, kills most early enemies in one or two hits. **It flies 850 units and no
  farther** (director, 2026-11-13 00:55 PST; `BoltRange`): less than the 900 units at which a post notices the wizard, so he
  cannot snipe a group that has not seen him. **One Bolt every 0.45 seconds** while the button is held (`BoltInterval`;
  director, 2026-11-13 14:40 PST: "too quick, shots should still matter". It was one every 0.2 seconds).
- **Spear of Sacrifice** (right mouse; named by the director 2026-11-12 15:25 PST; payment changed 2026-11-13 17:30 PST):
  **it is paid for in bones when he holds enough**: 3 bones (`SpearBoneCost`; director, 2026-11-13 22:20 PST; it was 4). **When he
  does not, he cuts down the 3 plain skeletons nearest him** (`SpearSkeletonCost`) and makes it of theirs; his bones
  stay. **Only plain skeletons are ever taken**, never a Guard or any other kind, and nobody who was sent away. A pale
  cut is shown through each skeleton taken (a stand-in for the slash animation). He shapes the bones into a spear,
  enchants it and throws it at the
  point he aims at, up to 900 units away and no farther than the first wall. Where it lands it explodes and deals 30
  damage to every enemy within 260 units: a grunt (10 health) dies outright, a Brute (80 health) loses less than
  half. One second between throws. **Locked at the start: bought at a shrine for 12 bones** (director, 2026-11-12 16:07 PST).
  All of these numbers can be changed while playing (`Wiz.Tune`).
  - **How it will look (later):** the wizard swipes at a group of skeletons and grabs them. They turn to dust and
    smoke, and the smoke hides the spear appearing in his hand. To be expanded when the animation is made.
- **Summon the Afterlife** (Q; the name may change): the starting spell. It spends the bones the wizard holds
  on plain skeletons, 2 bones each (`BonesPerSkeleton`; director, 2026-11-13 22:20 PST, "for now"; it was 3), which stand up beside him. One cast uses all the bones it can; `SummonMax` sets
  a number per cast instead (1 = one skeleton a cast), to be play-tested. It stops at the army cap, and does nothing
  while the shrine's menu is open.
- **Blink** (Space; director, 2026-11-13 11:30 PST, changed 12:20 PST and 2026-11-13 14:40 PST; built): a dash **the way he is
  moving**: holding A and pressing Space blinks left. Standing still, it goes the way he faces. It covers 700 units
  (`BlinkRange`) in 0.65 seconds (`BlinkSeconds`) **as a burst**: it starts at about 2,150 units a second and slows to a
  stop, after the Dash in the director's Nexus project (a steady 2,000 units a second for 0.3 seconds, in the held
  direction). **A direction held during the dash bends it**: held for the whole blink it carries him sideways 0.6 of
  the range, 420 units (`BlinkSteer`), at an even pace, so he can steer as much at the end as at the start. The blink
  still goes its full length, and holding the opposite way does not shorten it. Walls and water stop it, and he slides
  along them. **Nothing can hurt him during it**: enemies on him add nothing to Overwhelm, the meter holds where it
  is, and arrows fly through him. It can be cast again 3 seconds after it ends (`BlinkCooldown`). It cannot be cast
  while the shrine's menu is open. Later: an effect that makes him vanish for the dash.
- **Command the army** (F; director, 2026-11-13 11:30 PST; built 12:20 PST; changed 2026-11-13 14:40 PST): F enters command
  mode, and F again leaves it. Nothing happens with no army to send. In command mode:
  - the mouse wheel sets how many to send, from 1 to all that are with him (it starts at half);
  - right mouse steps through the kind to send: Any, Skeleton, Guard. **A kind the army has none of with him is passed
    over**;
  - **the number is shown to the right of the cursor, and the kind under it** (an image of the unit later);
  - left mouse sends them to the point under the mouse, and command mode ends. No Bolt is fired and no spear is
    thrown by the mouse while it is on. A point in a wall or over water becomes the nearest open ground.
  Those nearest the place go. At the place:
  - **They spread out.** The group stands over a circle that grows with its size (55 units times the square root of
    how many went, at least 90, at most 255: `CommandSpacing`, `CommandScatter`), evenly, not in a clump. The circle
    is drawn at the cursor before the send.
  - **They move about.** Each walks to another spot inside the circle every 1.5 to 4.5 seconds, like the army around
    the wizard.
  - **They find their way.** On the way there, and back from a chase, they go around walls and water by the shortest
    open way (over a bridge when the place is across the river). They do not yet prefer roads.
  - They fight any enemy that comes within reach of the place, however far the wizard is.
  - **Shot at from out of reach, they send some after the shooter**: two fifths of the group, those nearest it
    (`CommandRespondShare`), chase it down, up to 2,000 units from the place (`CommandRespondRange`). The rest hold.
    The chasers come back when it is dead or has got away. One shooter at a time.
  - **They rejoin him when he comes within 300 units of the place** (`CommandRejoinRadius`); sent to a place right
    beside him, they hold it until he has walked away and come back. A circle on the ground shows each held place.
- **Recall** (C): every sent unit comes back to the wizard.
- **Blink, changed 2026-11-13 15:40 PST (director):** it steers more at its start: six tenths of the sideways reach follows the
  burst and the rest goes at an even pace (`BlinkSteerEarly`), so holding a direction for the first fifth of the blink
  already gives about three tenths of the reach. **He cannot be seen** from the start of the blink until seven tenths
  of its time have passed (`BlinkHiddenShare`): he comes back as it slows. For now he simply is not drawn; an effect
  comes later.
- **Interact** (E): open or close the shrine's menu when standing in its circle.
- **Pause** (P): the pause menu: "PAUSED", a Resume button (P), a "Settings" button and a "Restart the run" button
  (R), all of which can be clicked, and the controls panel under them (every key, with the abilities' icons).
- **Start menu** (director, 2026-11-14 13:20 PST: "Add all the UI elements from the UI thread"; built): the game opens
  on it, paused behind it. "March" (Enter, or a click) starts the run; "Settings" (a click) opens the settings screen
  on its Controls tab. It is shown once for each time the game is opened: a restart goes straight back into the run.
  Automated runs skip it. `Wiz.StartMenu [0|1|controls]`; `Wiz.State` has `"menu"`.
- **Settings screen** (director, 2026-11-14 21:10 PST: "implement the new settings UI that the other thread made";
  built from the UI thread's settings hand-off). One panel, three tabs, opened only by the "Settings" button of the
  start menu (on Controls) or the pause menu (on the tab last used there, Display the first time). "Back" or P
  returns to the menu it came from; the game stays paused.
  - **Controls** (director, 2026-11-14 22:10 PST: "make controls changeable and storable in the cache so the browser
    remembers"; built): click a key, then press the new key or mouse button; Escape cancels. Twelve controls can
    move: the four move keys, Bolt, Use the shrine, Pause, Summon, Spear, Blink, Command, Recall. If the key is
    taken by another of them, the two swap. Kept for other things and refused: R (restart), Enter (march), Escape,
    the wheel, and the testing keys (1 2 3 H J K L M). A line under the list says what happened. "Reset controls"
    puts every key back. Wherever the HUD names a key (ability bar, pause menu, the shrine's prompt, Command's
    hint) it shows the key as it is now. The menus are always clicked with the left mouse button, whatever key
    Bolt is on. `Wiz.Key <Control> <Key> | capture <Control> | reset`.
  - **Sound** (director, 2026-11-14 22:10 PST: "make the necessary channels made for the settings page"; built): four
    channels, each a sound class in `/Game/Audio/Mix`: `SC_Master`, and under it `SC_Music`, `SC_Effects`,
    `SC_Interface`. **A sound is on a channel when its Sound Class is that channel's**; that is all a new sound needs.
    Each has a volume (0 to 100, steps of 5; defaults 80, 60, 90, 70) and a mute; the master also scales the other
    three. "Sound in a background tab": Keep playing or Mute (the default). "Play a test sound" plays one effect;
    letting go of the Master or Effects slider does too. "Reset sound". The volumes are set through one sound mix,
    `MIX_Settings`. `Wiz.Setting master|music|effects|interface <0-100>`, `mute <channel> [0|1]`, `background <0|1>`,
    `resetsound`, `testsound`.
  - **The director's sounds** are imported and on their channels, not yet played by the game: three music tracks
    in `/Game/Audio/Music` (looping, on Music), eighteen effects in `/Game/Audio/SFX` (on Effects; the click on
    Interface). Trimming them and deciding what plays them is the director's, in the editor.
  - **Display**: Quality (Low / Medium / High: the engine's quality levels 1, 2, 3; High is how the game looked
    before, and the default, where the hand-off said Medium); Render scale (50 to 100 %, steps of 10); HUD and menu
    size (80 to 130 %, steps of 5, taken when the slider is let go; the pause menu and this panel grow only as far
    as still fits the window); Danger edges (0 to 100 %, "Off" at 0); Show frame rate (a small number top right).
    Screen shake and Full screen are shown and marked "Later": the game has no camera shake, and asking a browser
    for full screen from the click is not wired. "Reset display" puts the tab back to its defaults.
  - Every change, keys and volumes included, applies at once and is saved (a save slot, "WizardSettings": in a browser, the page's storage),
    then loaded when the game opens. Quality and render scale are left as the engine has them until the player
    changes one. Automated runs neither load nor save.
  - `Wiz.Settings [0|1|controls|sound|display]`, `Wiz.Setting <quality|renderscale|hudsize|edges|framerate|reset>
    [Value]`; both reply with the screen and every setting. Getters on the Wizard Settings subsystem.
- **One key for each control** (director, 2026-11-13 11:30 PST): E no longer throws the spear, Space no longer summons, and
  Escape does nothing (a browser takes Escape to leave full screen, so the game would never see it there).
- **Testing keys, in every build** (director, 2026-11-13 11:30 PST): H shows or hides the debug stats, and 1, 2, 3 switch
  the camera (Angled, Top Down, Third Person). The stats start hidden in the shipped build.

## How a god falls: Overwhelm

The wizard has no ordinary health. He has an **Overwhelm** meter.

- Enemies that reach the wizard fill the meter while they are touching him. It drains when nothing is on him.
- Your skeletons intercept enemies before they reach you. A strong army keeps the meter empty.
- With no army, or one too small for the force in front of you, enemies pile on and the meter fills.
- When it fills, the god falls: he sinks under the crowd (about 2 seconds, eased), the final cut screen shows, and the
  run returns to the start.
- **Godbane elites** ignore your army and go straight for you, and they fill the meter much faster. They are marked
  clearly. Killing them first matters even when your army is strong.
- **The player must always know where it is coming from.** As the meter fills, the screen edge darkens and closes in
  from the side the pressure comes from (fading in and out over about 0.3 seconds), and a sound grows from that
  direction. A fall should never feel like a surprise.
- **It lasts a little longer** (director, 2026-11-14 11:19 PST; built and tested 2026-11-14 11:50 PST): the meter does not start
  to go down until 2 seconds have passed with no build-up at all (nobody on him, no hit), and then goes down at 0.2 a
  second (it was 0.25, with no wait). `Wiz.Overwhelm [Fill] [Drain] [Delay]` changes the numbers.
- **The commander weighs most on it** (director, same time): on the wizard he counts as 5 enemies; his slash fills 0.4
  of it, his spear 0.2; and while he is awake and within 900 units, everything on the wizard fills it 1.5 times as
  fast. A pulsing red crown in a ring, just right of the bar, shows that (the UI thread's design; no number beside it).
- It is easier early (few enemies, slow fill) but the wizard can fall at any point in the run.
- A difficulty setting may scale the fill rate and enemy counts later. Not in the first build.

## Army

**The Marksman** (director, 2026-11-14 11:19 PST; built and tested 2026-11-14 11:50 PST): the last friendly unit. It never
closes to fight: it keeps its place by the wizard and shoots every 1.5 seconds at an enemy within 900 units that no
wall hides. Archers first (the nearest), and one arrow kills an archer. With no archer in range, the enemy with the
most health. 10 to a brute (a skeleton's blow is 8) before the brute's armor, 8 to anything else. 15 health, frailer
than a skeleton. Enemies still at their posts are not shot.
- **How you get one is the agent's choice, to be confirmed by the director**: bought at the shrine for 6 bones, a
  sixth row like the Guard's (one, five, or all you can pay for). The other way would be an archer's bones raising one.
- Model `/Game/Army/SKM_Marksman` (160 tall, mannequin skeleton) and bow `/Game/Weapons/SM_Skeleton_Bow` are
  imported; the bow is not in its hand, and it has no shooting animation yet.
- `Wiz.AddSkeletons <Count> Marksman`; `Wiz.Tune Marksman*`, `ArrowArc`, `ArrowDropDistance`.

**The spear costs one skeleton** (director, same time; built and tested 2026-11-14 11:50 PST): 2 bones when he holds
them (what a skeleton is summoned for), else one plain skeleton is cut down. It was 3 and 3.

- **Bones make the army.** A dead enemy leaves bones where it fell. The wizard collects them by walking within 130
  units of them, which means walking toward the fight. Bones nobody collects fade 30 seconds after they leave the
  screen (`BoneLife`); **bones the camera shows do not fade** (director, 2026-11-13 15:40 PST). He holds them until he spends
  them. Skeletons can be destroyed.
- **A fallen ally leaves a third of its cost** (director, 2026-11-13 15:40 PST; `ArmyDropShare`): always a whole number. A
  plain skeleton (cost 3) leaves 1 bone; a Guard (cost 8) leaves 3, rounded from 2.67. Units spent on the spear, or
  on a purchase at the shrine, leave nothing.
- **Bones are the currency.** Summon the Afterlife turns them into plain skeletons; the shrine sells unlocks, better
  units and upgrades for them. Every bone is a choice between a bigger army now and a stronger one later.
- **Small ones are summoned, better ones are bought** (director, 2026-11-12 16:07 PST): the spell only ever raises plain
  skeletons. Stronger units, and armor or weapons for the plain ones, are bought at the shrine.
- **The army stays with the wizard (base version).** It does not roam off by itself. Each skeleton idles at its own
  spot inside a circle around the wizard (350 units), and every few seconds strolls to another spot, so the army
  drifts along with him instead of standing in set places. When he runs, they run to keep up (eased by distance).
- **It fights only what comes close.** A skeleton attacks an enemy that is within 300 units of it and within 500 units
  of the wizard (the leash). Past the leash it drops the fight and comes back. When a fight ends it idles where it
  stands instead of walking back to an old spot.
- **It stands in groups.** The army stands in 3 to 6 groups spread evenly around the wizard, like walls on every
  side of him: one group for every 7 skeletons (3 groups up to 21 skeletons, 6 groups at the cap of 40). Skeletons
  are shared out evenly, so no group is ever larger than 15, and a skeleton goes back to its group after a fight.
- A skeleton that is already fighting keeps its fight 80 units past both distances, so it does not start and stop
  when an enemy stands right on the edge.
- All of these distances can be changed while playing (`Wiz.Tune`), and the debug overlay draws the two circles.
- **The army counts toward his bone worth** (director, 2026-11-13 14:40 PST; built as a number only): each unit is worth a
  third of what it cost to make (`ArmyValueShare`). A skeleton cost 2 bones and counts 1 (and leaves 1 bone when it falls); a Guard cost 8 and three
  of them count 8. Bone worth is the bones he holds plus that. Nothing spends it yet; the debug stats and
  `Wiz.State` show it. **The army pays at the shrine** (director; built 2026-11-13 16:25 PST): bones held are
  always spent first, then the army with the wizard pays the rest and is sacrificed. 40 bones and a price of 50: ten
  plain skeletons pay the rest and die. With no bones and a large army the player may still buy; that is his choice.
  - A plain skeleton pays 1, a Guard 3. Plain skeletons go before Guards, those nearest the wizard first.
  - A Guard that pays more than is owed gives nothing back.
  - Units that were sent away do not pay. A Guard is never paid for with Guards.
  - Units spent this way leave no bones.
  - **The army only ever pays the rest of one purchase** (the agent's rule, so "Max" cannot empty the army unseen):
    x5 and Max buy what the bones pay for; with too few bones for even one, one can be bought with the army.
  - The menu shows "Army can pay: N" beside the bones, and a row the army would help pay for shows its price in red:
    "4 bones + 11 skeletons".
- **His presence makes the army strong** (director, 2026-11-13 15:40 PST: option 1; built). Within 600 units of the wizard
  (`PresenceRadius`) the army has its full strength. Farther away, a unit does half its damage (`AwayDamageShare`) and
  takes half as much again from every blow and arrow (`AwayDamageTaken`). So a sent group is a real cost. The circle
  is drawn in command mode and with the debug rings. If time permits: a commander of his own that carries a smaller
  presence with a sent group.
- **The army does not shove its own** (director, 2026-11-13 15:40 PST; built). A unit on its way walks around one of its own
  side that stands in its path. A standing unit is not pushed off its spot by one that walks. A unit whose spot is
  taken picks another, and when that is taken too it stays where it is. Enemies and the army still push each other.
- **Beside a wall or the water it stands in the open** (director, 2026-11-13 15:40 PST; built). When the wizard walks up to a
  wall or the river, a skeleton whose spot would be inside it or beyond it takes a spot on open ground on his side,
  instead of pressing at the wall and shaking.
- **Planned: who is in the army changes the wizard** (director, 2026-11-13 15:40 PST). Some kinds give him more for each one
  he has. Guards add a share of the most Overwhelm he can take. Plain skeletons give nothing. A later ally will add
  to his damage. **Each such bonus follows a curve** (director, 2026-11-13 16:25 PST): good up to a certain number of that
  kind, then each one more adds little, so past that point the trade is a bad one. The curves are edited in the "Army Bonus Curves" page
  (https://claude.ai/artifact/3cYxQELdq1HW66FRGzGqUd, private to the director), which shows where each stops being
  worth it and gives the numbers to paste back. Proposed for Guards, not built:
  +5% of the meter for each of the first 4 (20%), +2% for each of the next 4 (28%), +0.5% after that, never past
  35%. The curve is a list of points the designer can edit.
- **Draw limit:** at most about 40 army figures on screen.
- **Merging past the limit:** when the army outgrows the limit, skeletons merge into bigger ones. Ten skeletons become
  one **Brute** (larger, hits harder, counts as ten). Ten Brutes become one **Colossus**. The army keeps looking and
  feeling stronger while the number of figures stays low.
- The army's size is always shown as one number: its total skeleton count.
- Sacrifices and the bone spear spend from that total, breaking a big skeleton back down when needed.

## Enemies

**The commander's moves** (director, 2026-11-14 11:19 PST; built and tested 2026-11-14 11:50 PST). He always goes for the wizard
and never turns aside for the army; what blocks him, he slashes through. He no longer trades blows.
- **Slash**: the brute's slam made quick. The same growing red circle, 190 across the middle and 170 ahead of him,
  full after 0.5 seconds and landing 0.15 after that (a brute: 1.0 and 0.3). 20 damage to each of the army in it
  ("moderate": it kills a plain skeleton, not one in bone armor or a Guard), 0.4 of the meter to the wizard. He
  turns as he strikes, so it is always in front of him. A blink gets out of it.
- **Spear throw**: every 6 seconds, with the wizard 350 to 1200 units away and no wall between, he stands and draws
  back for 0.7 seconds, then throws at where the wizard stands. It flies through the army, stops at walls, and on a
  hit fills 0.2 of the meter and slows the wizard to half speed for 2.5 seconds ("Slowed" under the bar). A step
  aside or a blink dodges it.
- **Some throws predict** (director, 2026-11-14 17:20 PST: "Have the boss predict sometimes with his long straight
  attack. This doesn't mean they do it every time"; built): when he starts to draw back, half the time
  (`CommanderSpearLeadChance` 0.5, the agent's number) he aims at where the wizard will be when the spear gets there
  if he keeps going as he is, not at where he is. The warning lane shows the aim either way, so a change of direction
  once the lane is bright is still the dodge; walking straight on is not, against a throw that predicts.
- **The throw's warning** (director, 2026-11-14 11:40 PST: "Have a warning for the spear throw"; built): while he
  draws back, a red lane on the ground runs from him the way the spear will go, as wide as the spear reaches,
  growing to its full length (or to the first wall). It follows the wizard, then for the last 0.25 seconds it
  holds still and shows bright: that is where the spear flies, and stepping out of it then is the dodge.
  `Wiz.Tune CommanderSpearLock`.
- Numbers: `Wiz.Tune CommanderSlash*`, `CommanderSpear*`, `CommanderTouchWeight`, `CommanderAuraRadius`,
  `CommanderAuraFill`. `Wiz.SpawnEnemies 1 Commander` makes one.
- His model (`/Game/Enemy/SKM_Commander`, 286 tall, on the mannequin skeleton) and sword
  (`/Game/Weapons/SM_Commander_Sword`, 170 long) are imported; the sword is not in his hand yet.

**Arrows arc** (director, same time; built and tested 2026-11-14 11:50 PST). An arrow, the enemy archers' and the
Marksman's alike, is lobbed at where its target will be: it rises over the heads between (a fifth of the flight's
length at the middle) and can hit only within 140 units of its mark, on the way down. Whoever is there then is hit,
so a target that moves away is missed and another that moves in is not. It stops at its mark; walls still stop it.
The enemy archers' bow (`/Game/Weapons/SM_Wood_Bow`) is imported, not in their hands yet.

**Sizes beside the wizard** (director's table, 2026-11-13 16:25 PST; built. It replaces the first sizes of 11:30 PST, which
made the skeletons too small): every unit's height is a share of the wizard's height, so the crowd keeps its scale if
his changes.

| Unit | Height (1 unit = 1 cm) | Beside the wizard |
|---|---|---|
| Wizard | 220 | towering, godlike |
| Plain skeleton | 150 | about two thirds |
| Guard | 175 | about four fifths |
| Grunt | 170 | close to a Guard: an even fight, but smaller than the wizard |
| Brute | 240 | taller than the wizard: the threat |
| Archer, assassin | 165 | the agent's choice: about a grunt (not in the table) |
| Commander | 286 | the agent's choice: 1.3 of the wizard, so it stands over a brute (not in the table) |

The shares are `SkeletonSize`, `GruntSize`, `GuardSize`, `BruteSize`, `CommanderSize`, `ArcherSize`, `AssassinSize`
(`Wiz.Tune`). **Every character model is imported at its height in this table.** With figures this large the crowd
stands wider: units keep 70 units apart (was 50), the army's groups stand 280 units from the wizard with 100 units of
shuffle (were 240 and 60), and the roam circle is 420 (was 350).

**Every unit takes the room of its body** (director, 2026-11-13 17:30 PST: enemies at posts stood in each other after the
sizes grew; built): a body is a circle a quarter of the unit's height in radius (`BodyShare`; a brute's is 1.4 times
wider): skeleton 37, grunt 42, Guard 44, brute 84 units. Two units never stand closer than their two radii. A blow
reaches as far as the two bodies need, an enemy stops where its body meets the wizard's (50 units, `WizardBodyRadius`),
and an enemy within 150 units of him counts as on him (`TouchRange`, was 80). At a post, a unit does not stroll onto a
spot one of its own stands on, and two that cross slide past each other. The army's groups shuffle within 130 units,
300 from the wizard, and the roam circle is 440.

**Melee is one blow at a time** (director, 2026-11-13 16:25 PST; built): an enemy or ally in reach of its target strikes once
a second (`MeleeInterval`) for what it used to do in a second: a skeleton 8, a grunt 5, a brute 12, a Guard 14, the
commander 25. The first blow lands half a second after it gets in reach (`MeleeFirstShare`). **Units with a model swing as they
strike** (director, 2026-11-13 17:30 PST; temporary): the mannequin's unarmed swing, one second long, timed so the fist is
out when the damage lands (within a twelfth of a second). **Enemies on the wizard
still fill Overwhelm steadily, as before.**

**A unit stands still while it attacks, and does not turn** (director, 2026-11-13 22:20 PST; built; for every melee unit "at this
stage"): from the moment a blow starts until it lands the unit does not walk and keeps the aim it took as the blow
started; it aims again for the next blow. A unit in a blow is not pushed aside by others. If its target steps out of
reach the blow is off and it follows, with the blow still winding up, so an enemy that runs (an archer) is struck when
it is caught; without that a runner could never be hit. The animation still plays.

More abilities for some enemies, the commanders first, are to be designed next (director, 2026-11-13 11:30 PST).

**Nothing walks on water** (director, 2026-11-13 14:40 PST; built): no enemy and none of the army can stand on or cross
water, anywhere. A unit pushed into it by a crowd is put back on the bank it came from.

**Enemies fight the units sent at them** (director, 2026-11-13 14:40 PST; built): an enemy fights the nearest of the army
within 350 units of it (`EnemyEngageRange`) when the wizard is more than 700 units from it (`EnemyWizardRange`). So
a post attacked by sent units turns on them at once, and enemies passing a held place stop to fight it. With the
wizard nearer than that, enemies go for him as before and fight only what is in their way: that is what fills the
Overwhelm meter.

**Planned, at a later stage** (director, 2026-11-13 14:40 PST):
- **An attack roots the attacker.** An enemy or one of the army cannot move from the start of its attack until the
  attack is finished. The player can then step around behind; some enemies will have weak points at the back.
- **Move-sets for higher-tier enemies:** attacks that are shown before they land (the area to be hit lit in red) and
  can be dodged. To be discussed.

**Who the enemies are** (director, relayed by the assets session, 2026-11-13 16:35 PST): the kingdom's defenders, in its royal colors of crimson and gold. Not a
cruel tyrant's horde.

**Art route** (from the assets session, 2026-11-13 16:35 PST): every character is rigged to the UE5 mannequin skeleton (Tripo rig
type "Unreal Mannequin") and imported at its height in the table above. Crowd animations are to be baked with
AnimToTexture; one walk cycle is to be tested in the browser first. Source art lands in
`ArtIn/<Characters|Props|Materials|Audio|Concept>/`. The skeleton model has one bone connected wrong (minor, to fix
later).

| Enemy | Role |
|---|---|
| Grunt (built) | Weak, comes in groups. 10 health, speed 300. One Bolt kills it. Leaves 1 bone. |
| Archer (built 2026-11-13 12:20 PST) | Shoots from a distance and runs when charged. 6 health (a grunt has 10), speed 280. It stands off and looses an arrow every 1.6 seconds at the wizard or the nearest of his army within 800 units, when no wall hides them. An arrow does 12 damage to a skeleton (a grunt does 5 a second) and fills 12% of the Overwhelm meter when it hits the wizard (a grunt on him fills 8% a second). **It aims ahead** (changed 2026-11-13 15:40 PST): at where the target will be when the arrow lands, going by how the target moves when the shot is loosed, so walking on is no dodge and a change of direction is. **It is not a perfect shot:** each arrow is off by up to 4 degrees to either side (`ArcherSpreadDegrees`), which at its longest range misses a standing wizard about one time in twelve. When the wizard or any of his army comes within 380 units it runs, and does not shoot until it is 570 units clear. It never fights hand to hand. One in five of a post's grunts is an archer (`PostArcherShare`). A green cone for now. Leaves 1 bone. |
| Assassin (built 2026-11-13 12:20 PST) | Fast, frail and deadly. 8 health, speed 540 (the wizard runs at 600, a skeleton at 450). It never fights the army: it swerves around skeletons in its way and goes for the wizard. On him it counts as 4 enemies on the Overwhelm meter (32% a second). **Cloaked** (changed 2026-11-13 14:40 PST for more warning): the player starts to see it 800 units from the wizard (`AssassinRevealRange`) and sees all of it at 300 (`AssassinRevealFade` 500); it fades out the same way if he gets away from it. The army sees it only within 450 units of him (`AssassinArmyRange`). For now the fade is the cone growing from small to full size, not true transparency. A perk may change these distances later. A Bolt still hits it. One is sent from the nearest manned post every 40 seconds while the commander lives (`AssassinInterval`). A violet cone for now. Leaves 2 bones. |
| Brute (built) | **It slams** (director, 2026-11-13 22:20 PST; built): it no longer trades blows. When what it goes for is in front of it, a red circle 170 units in radius (`BruteSlamRadius`), its middle 190 units ahead of the brute (`BruteSlamDistance`), grows from tiny to full size in 1 second (`BruteSlamGrow`), turns bright red, and 0.3 seconds later (`BruteSlamHold`) the slam lands: 40 damage (`BruteSlamDamage`, a plain skeleton has 20) to each of the army inside, and 35% of the Overwhelm meter (`BruteSlamOverwhelm`) if the wizard is inside. From the first of the circle to the slam the brute stands and does not turn, so stepping out of the circle, or behind it, is the answer. **One slam never kills a guard or an armored skeleton** (director, 2026-11-14 00:15 PST; built): a guard loses half its full health (`BruteSlamGuardLeft` 0.5 is what is left), and once bone armor is researched a skeleton loses two thirds (`BruteSlamArmoredLeft` 1/3 is what is left), wherever the wizard stands. A hurt one can still die to it, and a second slam kills. A skeleton without armor still dies to one. Then it rests 1.2 seconds (`BruteSlamRest`). The numbers other than the 0.3 seconds are the agent's guesses. **Armored in front, weak behind** (director, 2026-11-13 17:30 PST): a blow or Bolt from within 60 degrees of dead ahead does half (`BruteFrontTaken`); from within 60 degrees of dead behind, double (`BruteBackTaken`); from the side, its full damage. It turns toward what it goes for at 140 degrees a second (`BruteTurnDegrees`), so the wizard can get behind it. The box turns with it. Its weak point is no longer marked (director, 2026-11-14 17:20 PST: "For brutes, remove their weakpoint indicator"): the back still takes double, with nothing drawn on it. Slow and tough; chews through skeletons. 80 health (eight Bolts), speed 190, more than twice a grunt's damage. A large dark block with a wider health bar. From 45 seconds into a run each wave brings one, and one more for every further minute, up to four. Leaves 3 bones. |
| Godbane elite | Ignores the army, targets the wizard, fills Overwhelm fast. |
| The Tyrant | Final boss, with waves of goons. |

**Bones by enemy.** Every kind of enemy leaves its own number of bones in one pile: a grunt 1, a Brute 3
(`BonesPerGrunt`, `BonesPerBrute`). Stronger enemies will be worth more.

**Kinds of skeleton.** The plain skeleton (20 health, plus 10 for every level of bone armor) is summoned for 3
bones. The **Guard** (60 health, almost twice the damage, taller and darker) is bought at the shrine for 8 bones.
The spear never takes a Guard while a plain skeleton is left.

**How enemies move.** Every enemy charges the wizard first. It turns on a skeleton only when that skeleton is in
its way (within 90 units and in front of it). An enemy that has reached the wizard stays on him. Each enemy bends its
approach to its own side and has its own speed, so a wave arcs in and surrounds instead of arriving in a line. Enemies
find their way around walls.

**Enemies hold their ground** (director, 2026-11-12 16:07 PST; built 2026-11-12 20:15 PST): enemies do not chase the
wizard across the map. The player sets the pace by moving forward.

- **Posts.** A level has posts, each manned by a set group. A group holds its post until the wizard or one of his
  army comes within 900 units of any of them (`WakeRadius`), or one of them is hit or killed. Then the whole group
  charges.
- **Losing him** (director, 2026-11-14 00:52 PST; built and unit-tested 01:10 PST, not yet watched in play): an enemy from a post or a
  road patrol sees the wizard within 1300 units (`SightRadius`) when no wall or building is between them; water does
  not hide him. While any one of a group sees him, the whole group knows where he is. When none does, by distance or
  around a corner, each goes to the spot he was last seen at, looks about for 3 seconds (`SearchSeconds`), then runs
  back to where it stood when it noticed him and takes up its post or patrol again, where it can notice him again.
  Seeing him at any point ends the search. While searching it still fights any of the army near it. It gives up on
  reaching the spot after 10 seconds (`SeekSeconds`) and on getting back after 30 (`ReturnSeconds`). A wall now also
  keeps a group from noticing him or his army in the first place. **Those sent after him never lose him**: waves, the
  raids from the next post, assassins, and a group woken by `WakeAll`. `SightRadius` 0 switches all of it off. The
  numbers other than "a few seconds" are the agent's guesses. `Wiz.State` has `searching` and `returning`.
- **Life at a post** (director, 2026-11-13 00:55 PST): the world should feel alive. About two in three of a group stroll
  about within 220 units of where they were posted, stopping a few seconds between walks; the rest stand
  (`PostWanderRadius`, `PostWanderSpeed`, `PostIdleShare`). The commander stands.
- **Road patrols** (director, 2026-11-13 00:55 PST): groups of 5 grunts walk the roads end to end and back, at a walk
  (150 units a second). They notice the wizard the way a post does. The Outskirts have four patrolled roads: the east
  road below the shrine, the east road above it, the middle road over the bridge, and the bottom bridge with the west
  road. A patrol that is killed is replaced after 40 seconds, at the end of its road farther from the wizard and
  never within 1800 units of him, as long as the commander lives (`RoadPatrolSize`, `RoadPatrolRespawn`,
  `PatrolWalkSpeed`).
- **Hunting parties.** While the commander lives, 5 grunts come straight for the wizard from the next manned post
  every 25 seconds (`PatrolInterval`, `PatrolSize`; 0 switches them off). With the road patrols, a player who camps
  or leaves the keyboard is not safe.
- **Numbers.** The Outskirts hold 14 posts and the commander's escort: 167 enemies, and 20 more on the roads. The cap
  is 320 enemies at once (`MaxEnemies`). More enemies mean more bones, so research is in reach.
- **The commander** caps the area: 400 health, a hard hitter, a large black and gold block with the widest health
  bar, standing with an escort at the way out. He leaves 15 bones. When he dies the area is taken, the HUD says so,
  and the patrols stop. What happens next (the gate to the next area) is not built.
- The old ring waves that charge from every side remain only in the practice level (`Wiz.Map Practice`).

**Health bars.** An enemy shows a small health bar above it only for 2.5 seconds after it takes damage, fading out
over the last half second. Nothing is drawn for an enemy that has not been hit, so the bars cost almost nothing.

## Obstacles

Walls and pillars block the wizard, every unit and the Bolt. The first build has a practice layout of ten around the
start (a long wall, a wall across, a corner, a gate with a gap, four pillars) to feel what moving around cover is
like. `Wiz.Obstacles` switches them off and on. Real layouts come with the three areas.

## Shrines and sacrifice

**Built.** A shrine is a dais with a glowing pillar and a circle on the ground (345 units). Standing in the circle
shows "E: use the shrine". E opens its menu.

- **Time runs at half speed while the menu is open** (director, 2026-11-12 20:15 PST; `ShrineTimeScale`). The wizard can
  still walk. This replaces the earlier rule that the fight went on at full speed.
- He cannot use Bolt, Summon the Afterlife, the Spear of Sacrifice or Blink while it is open.
- The menu closes with E, with a click outside its box, or by walking out of the circle. Falling closes it too.
- The menu stands at the right of the screen so the wizard stays in view.

**Research: bought once** (director, 2026-11-12 20:15 PST). The shrine's upgrades are research: better armor, better
weapons, better stats, and abilities. Each is bought one time with bones and lasts the run.

| Research | Cost | What it does |
|---|---|---|
| Spear of Sacrifice | 12 bones | Unlocks the spear (right mouse or E). |
| Bone armor | 15 bones | +15 health for every plain skeleton, those already in the army too. |
| Bone blades | 15 bones | +4 damage a second for every plain skeleton. |
| Dark focus | 15 bones | +5 damage for Bolt. |

A research that is done shows "Researched" and has no button.

**Units: bought in bulk.** A Guard skeleton costs 8 bones. Its row has three buttons: x1, x5 and Max (as many as
the bones and the army cap allow), so nothing needs repeated clicking.

**Stats are kept apart from research** (director, 2026-11-12 20:15 PST): raising armor, health and base damage step by
step is its own thing and will be a second tab of the menu. Not built yet.

All costs and amounts can be changed while playing (`Wiz.Tune SpearUnlockCost`, `ArmorCost`, `ArmorHealth`,
`BladesCost`, `BladesDamage`, `FocusCost`, `FocusBoltDamage`, `GuardCost`).

**The HUD never lists what cannot be used** (director, 2026-11-12 20:15 PST): the spell's line shows only when a cast
would raise something, and the spear's line only once it is researched and the army can pay for it.

**When the shrine can be used** (director leaning, 2026-11-12 15:25 PST): at all times, with the real chance to use
it in the lulls between fights.

- The sacrifice is shown (later): the chosen skeletons crumble to dust and stream into the shrine (about 1 second, eased).

## Structure

**Taking the kingdom back** (director, 2026-11-12 16:07 PST): the player charges into his old kingdom and goes through the
tyrant's forces to retake it. He moves through the levels and progresses a part at a time; the enemies hold their
ground instead of hunting him.

**Rounds, not endless waves** (director, 2026-11-12 15:25 PST): each area has a set number of enemies and ends with a
powerful enemy that caps it: a boss, general or commander. Not built yet.

**One level: the Outskirts** (director, 2026-11-14 10:39 PST: "condense this project down to just being the one level
considering the remaining time left as we still have sound, vfx, and other things to complete"). The game is the
Outskirts alone. Inside the Walls, the Castle and the Tyrant are cut; the commander at the gate is the last enemy.
**The game finishes when the wizard enters the kingdom gates** (director, 2026-11-14 10:42 PST), and the gate opens
only once the commander is dead (director, 2026-11-14 10:49 PST: "Yes, commander first"). **Built and tested 2026-11-14 11:50 PST** (director: "Do not make a build, only do the changes").
- The gate is a Kingdom Gate actor in the level (`Systems`), like the shrine and the posts: move it, turn it so its
  arrow points the way through, and set `Width` (600) and `Depth` (300). It is ground to walk onto, not a wall: a
  shut gate stops nobody, it only ends nothing. Its marking on the ground (the shrine's pulsing ring, stretched,
  until it has a look of its own) shows in the editor always and in the game once the gate is open.
- **It is not in the level yet.** After the next build, `Tools/Level/place_gate.py` (or the layout actor's "Place
  Gate As Actor" button) puts it 900 units past the commander's post, on the line the road was taking. Until then
  the game uses that same spot from code.
- **When the commander dies** (the UI thread's gate message, version 4, https://claude.ai/artifact/565RJ2nf3SCjunzuSx8cJ6;
  built 2026-11-14 15:15 PST): a banner just under the Overwhelm bar fades in over half a second, stays for three,
  and fades out: a broken crown, "The commander has fallen", and "The kingdom gate is open" in the gate's orange
  ("The Outskirts are yours" in a level with no gate). No bars: the HUD stays clear. Then, never together with the
  banner, a line under the Overwhelm bar says "Enter the kingdom gate" with the distance in metres, and a pointer
  shows the way: an orange arrow at the screen's edge with "Gate" while the gate is out of view, and a bobbing mark
  over the gate with its distance once it is in view. `Wiz.TakeArea` plays it without killing the commander.
  The arrow keeps 200 from the top and bottom (the design says 150, which put it on the line that says where to go),
  and it is two strokes, not a filled arrowhead.
- Entering stops the fight where it stands and shows the final cut screen (below). A wizard who is falling or
  mid-blink does not enter.
- `Wiz.OpenGate` opens it with the commander alive; `Wiz.Goto gate` puts the wizard in it; `Wiz.State` has `gate`
  (none, shut, open, entered). Everything below
about three areas is kept as the earlier plan, not as work to do.

Earlier plan: one linear run of about 10 to 15 minutes, three areas:

1. **The Outskirts**: farmland and townhouses outside the walls. Grunts, then the first Brutes. Learn bones, the
   spell and the shrine. Ends at the gate with its commander.
2. **Inside the Walls**: streets with houses, blacksmiths and armorers. Tighter ground, tougher and better-armed
   enemies. Ends at the castle door with its commander.
3. **The Castle**: halls and stairs up to the throne room, where the tyrant waits.

A shrine stands in each area. Falling at any point returns the run to the start.

## The map

Direction (2026-11-12 14:55 PST): a dark medieval land. Dark-toned rock, dead and blackened trees, fire, rain.

**Night** (director, 2026-11-14 13:20 PST: "Change the lighting to give a more night time feel while still being able to
see in-game"; done in the level by `Tools/Level/night_lighting.py`, whose numbers are at its top). The sun is a moon:
3 lux (it was 6), cold blue, still casting the shadows. The sky is darkened to navy; the sky light is raised to 3.5
and tinted blue, so shadows are never black. Exposure is fixed (no eye adaptation, +1.2 stops), so the night is not
brightened back to day. Fog is dark blue, colors are a little muted, and the corners are darker. These are the
level's own actors (the light, the sky light, the atmosphere, the fog, and a "Night Look" post process volume), so
they can be changed in the editor. Not yet seen in a browser.

**Mood.** Night or heavy overcast, cold blue-gray light from above, warm orange only where there is fire. The ground
is dark mud, ash and wet stone. Units must stay readable from the Angled camera, so the ground and props stay
darker and less saturated than the bone-white army, the red enemies and the purple wizard.

**How levels are made** (director, 2026-11-12 20:15 PST): the director draws the level in 2D; Claude blocks it out in
Unreal with basic shapes; the director then dresses it with meshes. The Outskirts are blocked out from the first
drawing (`Wiz.Map Outskirts`, the level the game starts in): the river with two bridges, the walkways, the
farmland, five graveyards, about 120 houses, seven medical buildings, the boat dock, trees and twenty lamp posts.
One pixel of the drawing is 15 units (10 at first; raised by the director on 2026-11-13 00:55 PST so that a house is a
house beside the wizard, who is larger than a man but not the size of a building; `Wiz.MapScale` changes it while
playing). Houses stand 420 units tall. Water stops everything that walks, but Bolts and the spear fly over it. The
edges of the drawing are walls. The layout is plain data in `Sim/LevelLayout.cpp`, checked by tests (the wizard
can walk from the start to the shrine, every post and the commander; only the bridges cross the river).

**The three areas as places.** One road runs through all of them, so the run reads as a charge on the tyrant.

| Area | Place | What blocks the way | Landmark for the shrine |
|---|---|---|---|
| The Outskirts | Burned fields and townhouses outside the walls. The most open ground, so the first army has room. | Fences, carts, low walls, a few houses, dead trees. | A roadside chapel ruin with a brazier. |
| Inside the Walls | Streets and small squares between houses, blacksmiths and armorers. Narrow, with choke points. | House blocks, market stalls, forges, barricades. | The town square's well or statue. |
| The Castle | The courtyard, then halls and a stair, then the throne room. | Pillars, inner walls, doors. | A chapel alcove before the throne room. |

**How it is built.**

- Every blocking prop is a box to the game, exactly like today's practice walls. The art sits on top of the box.
  So the layout can be built and played with gray boxes first, and dressed afterwards without changing how it plays.
- Rules for the layout, learned from the practice walls: gaps between obstacles at least 200 units wide (a group of
  skeletons fits through), no dead-end pockets deeper than a few hundred units, and open ground around each shrine.
- Fire is the light: torches and braziers mark the road, the shrines and the way forward. A player who is lost
  walks toward fire.

**What the browser allows.**

- A small kit, reused: about 3 rocks, 2 dead trees, 1 gravestone set, 1 ruined wall set, 1 palisade, 1 cart wreck,
  1 torch, 1 brazier. Each is one low-polygon mesh drawn many times as instances, like the crowd.
- No moving lights that cast shadows. Fire is a glowing material plus a few unshadowed point lights, with the cap to
  be measured in the browser; the flicker is done in the material.
- **Lighting is pushed as far as the browser allows** (director, 2026-11-12 16:07 PST: it should look like a game, not a
  phone game). Baked lightmaps for the map (which needs a real level with static meshes, instead of today's boxes
  made at run time); one moon or overcast directional light whose shadows the wizard and the crowd cast; baked
  ambient light; reflection captures; height fog; bloom, color grading and vignette. Each step is measured in the
  browser for frame rate and download size before it is kept. Not available on the web: Nanite, virtual shadow
  maps, ray tracing; Lumen only in an experimental tier that is too heavy for a crowd.
- Rain is one cheap effect that follows the camera (streaks near the camera and a wet sheen on the ground material),
  not rain over the whole map. Fog is the engine's height fog, which also hides the map's far edge.
- No dense foliage, no trees with leaves, no water surfaces.
- The size budget for all map art together is about 30 MB of the download; the build is checked after each batch.

**Where the pieces come from.** Meshes from the jam's partner services (Tripo for rocks, trees, ruins and props),
with a prompt from Claude for each piece. Claude places them, builds the collision boxes and the lighting, and
measures the frame rate. Order: gray-box layout of the Outskirts first, then the kit, then dressing, then the other areas.

**The wizard's model** (director, 2026-11-13 10:25 PST): the Wizard King, made in Tripo (version 2: crown, armor, purple robe),
replaces the cylinder and sphere. He stands 220 units tall (a grunt is 50, a house 420; `Wiz.ModelHeight` changes it,
`Wiz.Model 0` brings the shapes back). He is on the standard UE5 mannequin skeleton and is moved by the mannequin's
default animation blueprint (`ABP_Unarmed`: idle, walk and jog in eight directions), so any animation made for the UE5
mannequin plays on him. He faces where he aims, so walking away from the aim plays the backward and sideways steps.
Characters rigged by Tripo are named like the UE5 skeleton but their bones point another way: every such model goes
through `Tools/Art/clean_rig_fbx.py` (Blender) and `Tools/Art/import_mannequin_character.py` before it can share the
mannequin's animations.

## Opening scene (if time and size allow)

Asked for by Screeps on 2026-11-11 22:12 PST: show the tyrant driving the wizard king out of his kingdom, to give
the game its context.

- Planned form: played in the engine with the game's own shapes and crowd, about 15 to 20 seconds, skippable with any
  key. The wizard stands in his throne room, the tyrant's horde floods in, the Overwhelm meter fills, the god sinks,
  and the game opens in the Outskirts. It adds almost nothing to the download and teaches Overwhelm before the player
  can fall to it.
- Built on day three after the Tyrant and the final cut screen. It is the first thing cut if day three runs late;
  the fallback is two lines of text over the first seconds of the Outskirts.

## Final cut screen

Shown when the god falls and when the Tyrant is beaten: skeletons raised, skeletons cut (sacrificed and detonated),
perks taken, enemies killed, time, and how far you got. One button: march again.

**Built 2026-11-14 15:15 PST to the UI thread's design** (director: "Build the final cut screen and the kingdom is
open message"; https://claude.ai/artifact/LYKXUy3ofaXA8sD73ir1mi, round 2). The game darkens behind it over a second.
Then, one after another, each rising a little as it fades in: "FINAL CUT" (purple, spaced out, a fading line to each
side); the title **"The Outskirts are yours"** in gold, the only gold on any screen; "Taken in m:ss"; the tally
panel; the buttons.
- **Tally**: four numbers that count up (skeletons raised, sacrificed in red, enemies killed, army at the gate),
  then the ledger "Of the N you raised": one bar in three parts that grow in, kept (the army at the gate, bone),
  cut for power (sacrificed, red), fell in battle (the rest, grey), with the three counts under it. Bought units
  stand at the gate without having been raised, so "fell" stops at zero.
- **Buttons**: "March again" (R, or a click) restarts the run; "Enter the kingdom" is locked, with a padlock, and
  says why when the mouse is over it ("The kingdom's gates stay sealed for now. / This part of the tale is still
  being written.").
- Left out: the title's soft glow, and the darker corners (the dark is even).
- The fall still restarts the run with no screen.

The first version (2026-11-14 10:49 PST), replaced by the above: the screen darkens over one second (eased) and shows
"Final cut", "The kingdom is yours", the time, skeletons raised, skeletons sacrificed, enemies killed, the army's
size at the gate, and "R: march again". The fall still restarts the run by itself with no screen. Perks taken and
skeletons detonated are not counted on it yet.

## Camera

- **Buildings are cut away around the cursor** (director, 2026-11-13 16:25 PST; built): inside a circle around the cursor on
  screen, walls, houses and trees are drawn as a thin speckle (about one pixel in eight), so the player can see and
  aim at enemies behind them. The circle is 0.13 of the screen's height in radius, with a soft edge
  (`Wiz.SeeThrough [Radius] [X Y]`; 0 switches it off). It is a cut, not true transparency: that keeps the cost low
  in a browser. Low things (docks, lamps, water) are left whole.

**Angled** view above and behind the wizard, chosen after the first playtest (2026-11-12 02:00 PST). The other two
cameras (Top Down, Third Person) stay reachable in development builds only, through `Wiz.Camera` and the keys 1, 2, 3.
H shows or hides the debug overlay. None of these keys exist in the shipped build.

**Under consideration** (director, 2026-11-13 00:55 PST): letting the player move the camera in the angled view, and a 3D
view now that the level has buildings. Not to be built until the scale of everything is settled.

## Look and sound

**Round of 2026-11-14 17:20 to 18:07 PST** (director's list; all built, compiled and tested, none of it played by hand):
- **The mouse** is the plain pointer on any menu (start, pause, shrine, final cut) and the crosshair in the game.
- **Attacks are timed by the "Fire" notify** the director put on each attack animation (a montage notify named
  "Fire": archer 1.19 s of 1.70, brute 0.89 s of 2.27, wizard 0.80 s of 2.28).
  - *The crowd.* The simulation still decides when a blow lands (so the game plays the same with or without models),
    and each kind's animation is laid over one turn of its attack so that its Fire moment meets that blow: an
    archer's or Marksman's arrow leaves at Fire, a brute's slam comes down at Fire and its animation starts when the
    red circle does. A kind has six hidden figures playing the animation a sixth of a turn apart, and each figure
    on screen copies the nearest one, so the timing is right to within a twelfth of a turn (0.13 s for an archer,
    0.19 s for a brute). The Marksman has no animation of its own and uses the archer's. The commander has none.
  - *The wizard.* A Bolt now leaves `BoltWindup` (0.2 s) after the cast starts, at the animation's Fire moment: the
    start of the animation is played faster to get there in that time, and the rest plays out at its own speed.
    Held, he still casts every `BoltInterval` (0.45 s). `Wiz.Tune BoltWindup 0` makes it instant again.
- **The Bolt animation is on the upper body only**: from `spine_01` up. The legs keep the animation blueprint's
  idle, walk and run. Done in C++ (`UWizardAnimInstance`, now the parent class of `ABP_Unarmed`; the blueprint's
  graph is untouched), because the graph cannot be edited by a script. The montage is read, not played, so notifies
  on it other than Fire (a sound, say) do not fire yet.
- **Bows**: the enemy archer holds `SM_Wood_Bow` and the Marksman `SM_Skeleton_Bow`, on `Bow_Socket`.
- **What flies and what falls** (director, 2026-11-14 18:00 PST): the archer's arrow, the Marksman's arrow and the
  wizard's thrown spear are the director's models (90, 90 and 200 cm; the commander's spear is the archer's arrow,
  larger), and dropped bones are the bone pile model (a brute's or Guard's pile is 1.6 times the size).
- **The brute holds the steel hammer** (`SM_Steel_Hammer`, 120 cm) on `Hammer_Socket`, which the director placed
  (2026-11-14 18:45 PST).
- **Round of 2026-11-14 18:45 PST** (director's list; built):
  - **The Bolt's throwing arm**: the cast was laid on the hips as they were (walking, leaning, turned), so the arm
    went wide. The upper body's first bone now keeps the direction it has in the animation, measured in the figure's
    own space, so the throw goes straight ahead whatever the legs do.
  - **Roads** are what is painted on the landscape: the 33 road blocks are gone from the level.
  - **Bridges** are the director's bridge model over each bridge block (the block stays as the way across and draws
    nothing). The model is turned to cross the river and pressed to a third of its height, so its deck lies near
    the ground the crowd walks on. The water lies 3.5 over the level's floor, under the deck.
  - **The commander's spear** is its own model (260 cm).
  - **The Marksman's picture** is in the shrine's sixth row.
  - **Water**: a rippled normal map made for it (`Tools/Art/make_water_normal.py`) slides downstream at two sizes
    and speeds over a slow swell; the sides of ripples show the night sky and some glint toward the moon; greener
    shallows, mud at the banks. Still opaque and cheap.
  - **Houses block only their walls**: a house turned on the map used to block one big box around itself (boxes
    lie along the level's X and Y), far past its corners. It is now a grid of small boxes that follows its walls
    (`BuildingCellSize` 220). The mesh's own solid box, which stops the wizard's body, is its walls without its
    eaves (0.85 of its bounds).
- **The river** is one smooth mesh (`/Game/Level/SM_River_Water`, "River Water" in the level) drawn along a curve
  through the river's pieces, with water that moves: dark, cold, ripples drifting downstream that catch the light,
  mud at the banks (`M_River_Water`, plain math, opaque). The pieces stay as what stops feet, so the banks the
  wizard is stopped at are still the stepped ones, up to about a wizard's width off the drawn bank at a corner.
  Run `Tools/Level/make_river_water.py` again after moving river pieces. River pieces turned sideways (the director
  puts them beside the bridges) are not on the river's line and are skipped: on 2026-11-14 the curve ran through
  them and zigzagged at each bridge, and the mesh was put back to the one from before that run (director: "revert
  whatever water change you did"). The five pieces past the west edge have no drawn water until the script is run.
- **The kingdom's wall** (director, 2026-11-14 21:10 PST: "Import all the meshes in this directory"): four models
  in `/Game/Kingdom`, imported only, placed by the director on the east side: `SM_Castle_Wall` (1890 long, 952
  deep, 1000 tall), `SM_Castle_Gate` (1880 by 856, 1000 tall), `SM_Castle_Gate_Door` (one half: 490 wide, 800
  tall), `SM_Castle_Tower` (723 by 703, 1200 tall). They have no collision yet and are not cut away by the cursor.
- **The battlefield has no walls** (director, 2026-11-14 19:40 PST: "Replace the walls with just a soft barrier";
  built). One Battlefield actor (folder "Systems", an orange wire box in the editor) marks its edge where the four
  walls stood. Outside it "RETURN TO THE BATTLEFIELD" shows with the time left; after 6 seconds outside
  (`BattlefieldWarnSeconds`), or at once 900 past the edge (`BattlefieldFarDistance`), the mission fails: he falls,
  the screen says "You abandoned the battlefield", and the run restarts. Not in god mode. Past the west, south and
  north edges stand 21 more houses and 16 lamp posts (folder "Edge Dressing"), and the river runs on five pieces
  further west. The east is left for the kingdom's wall and gate. A Wall actor still works where one is placed.
- **Round of 2026-11-14 19:40 PST** (director's list; built):
  - **The web build was missing the bows and the shooting, slam and casting animations** (and the arrow, spear,
    bone pile and hammer models): they are loaded by name, and their folders were not in the list of what a
    package must include. `/Game/Animations`, `/Game/Weapons` and `/Game/Props` are in it now.
  - **Marksmen stand to shoot**: for the last 0.8 s before each arrow (`MarksmanDraw`), with something to shoot at,
    a Marksman does not follow the wizard; after the arrow it catches up. **They keep away**: each time an enemy
    comes within 260 (`MarksmanKeepAway`) a Marksman decides once, with a 60% chance (`MarksmanRelocateChance`),
    to back away from it for 1.2 s (`MarksmanRelocateSeconds`), not shooting while it does.
  - **Only plain skeletons are sacrificed or counted**: the shrine never takes a Guard or a Marksman as payment, and
    the army's worth in bones is its plain skeletons only.
  - **The cursor's circle cuts away houses and trees again**: the four house materials and the tree pack's two
    master materials read the circle from `/Game/Level/MPC_SeeThrough`. Walls are no longer cut away, and nothing
    else is (bridge, lamp posts, shrine, and the castle wall and gate to come) unless its material is added to
    `Tools/Art/add_see_through.py`.
  - **What blocks is the collision made in the mesh editor**: a house blocks the convex pieces the director made
    for it, as they lie on the ground (all of one house on one grid of 220, each row a few long boxes). A bridge's
    wide pieces are its deck, walked on; its narrow ones (rails, posts) block. On a bridge the crowd's figures
    stand at the deck's height (a trace down onto the bridge's collision), not on the ground under it.
  - **Imported**: `SM_Dagger` (45 cm). Not held by anyone yet.
- **Ground**: Landscape mode's Paint tab has two more layers, "WildGrass" and "DirtySoil", the two Megascans
  surfaces (color and normal, one repeat every 4 meters). What was painted before is unchanged.

**The HUD** (director, 2026-11-14 11:40 PST: "grab from the artifact/icons and implement that into the game"; built
from the UI thread's round 2, https://claude.ai/artifact/7EDJFvpFFXC9jM5DErzora). Sizes are pixels at 1080p and
scale with the screen.
- **Top left**: "ARMY", the army's size as the biggest number on screen (56), what was sent beside it in gold, and
  under it the bone icon with the bones held.
- **Top center**: "OVERWHELM" over a bar 520 by 22 with marks at each quarter; the commander's crown 22 to its right.
- **Bottom center**: abilities as square slots (92; Command and Recall 72, after a divider), each with its key on
  the corner and its name under it: Summon (Q, with "+N" for what it would raise), Spear (right mouse), Blink
  (Space), Command (F), Recall (C). A slot that cannot be used at all is not shown, so the bar shrinks and stays in
  the middle. A cooldown darkens the slot, wipes clear as it runs out, and counts down in it.
- **Red edges**: each edge now closes in by how many enemies are on him from that side, so being surrounded closes
  in from all four at once (it used to lean toward one direction only).
- Icons: `/Game/UI/T_Icon_Summon`, `_Spear`, `_Blink`, `_Command`, `_Bone` (256), made by the director with fal.
- **Menus** (the UI thread's menus round 2, https://claude.ai/artifact/D7STJpyFnyZAD4NpAZfmV1; built 2026-11-14):
  - Every menu panel is near-black with a parchment frame. Buttons: purple ("royal") for the main choice, a dark one
    with a parchment border for the second, deep red with "Sacrifice" when the army pays, grey when it cannot be used.
  - **Shrine**: a panel 600 wide at the right. "SHRINE"; the purse (bone icon, bones held, "Army can pay" in red);
    "RESEARCH"; then one row for each thing sold, 96 tall: its picture (56), its name, its price and what it does,
    and its button (150 by 48), or x1 / x5 / Max (64 by 48) for a unit, with "Can buy N" under the price. Six rows
    (the design has five; the Marksman is the sixth and has no picture yet, so its box shows "M"), which makes the
    panel 800 tall. Footer: how to close it, and "Time runs at half speed".
  - **Pause** and **start**: see Player. The start menu's picture is the director's (`/Game/UI/T_Start_Background`),
    darkened from the left under the words. The game is called **The Bone King** (director, 2026-11-14, relayed through the assets thread), for now.
  - Pictures: `/Game/UI/T_Icon_Armor`, `_Blades`, `_Focus`, `_Guard` (256), made by the director with fal.
  - **Titles and labels are Cinzel Bold** (director, 2026-11-14 14:17 PST: "Yes, download"; SIL Open Font License,
    from https://github.com/NDISCOVER/Cinzel, kept with its license in `Tools/Art/Fonts/Cinzel`), spaced out as
    designed: "ARMY", "OVERWHELM", "SHRINE", "RESEARCH", "PAUSED", "CONTROLS", and the start menu's title and the
    line over it (the design has that line in Cinzel Medium, which has no file of its own: it is Bold).
  - **Different from the design, for now**: the line "Sound starts with your first click" is left off until there is sound; the spear's hint says "by a skeleton"
    (the price is one now, the design said 3).
- **Different from the design, for now**: the cooldown is a wipe from
  the top, not a clock sweep; Recall is a drawn arrow, not a picture; Bolt has no slot.

- **Shapes are for testing only.** For the first days, while the idea is being tested for fun: wizard: a tall
  cylinder with a sphere head. Skeleton: a small pale capsule. Enemies: colored boxes. Elites: gold cones. Shrine: a
  ring and a pillar. Tyrant: a large dark box. Walls: gray boxes.
- **The assassin is a model** (director, 2026-11-13 22:20 PST; built): `Content/Enemy/SKM_Assassin` at 165 units, through the same route. It
  has no swing figures (it never trades blows) and grows into view as its cloak drops. **The archer and the brute are models too** (director,
  2026-11-13 23:10 PST; built): `SKM_Archer` at 165 and `SKM_Brute` at 240. Neither has swing figures: the archer shoots and the
  brute slams, and there is no animation for either yet, so they stand through it. The glowing knot still marks the
  brute's back.
- **The Outskirts is laid out in the level, not at run time** (director, 2026-11-14 02:23 PST: "Nothing should be done at runtime that
  cant be seen in the editor pre-play"; built). This replaces what the next entry says about pieces being drawn by the
  layout actor, about house boxes, and about what is "still code".
  - **Shrine**: one Shrine actor, the shrine model, solid. `Range` (700) is how close the wizard must be; a ring on
    the ground shows it and pulses, in the editor (with Realtime on) as in the game. It stands 1275 units from where
    the stand-in stood, at the nearest spot clear of roads, lamp posts and buildings.
  - **Posts**: 14 Enemy Post actors and a Commander Post, in `Systems/Posts`. Each has `Grunts`, `Archers`, `Brutes`,
    `Commander` and `Radius`; a red disc shows its ground and a label says who stands there (editor only). The group
    starts spread evenly over the ground and walks about all of it. A post's ground is at least 300 units and grows
    with the group (115 times the square root of its number); the label says "(tight)" when it is set smaller. No
    two posts' grounds meet, and houses were kept off them. Archers are now a number of their own on the post, not a
    share of its grunts.
  - **Patrol roads**: 4 Patrol Road actors, each a spline of straight stretches, with `Patrol Size`.
  - **Start**: the level's Player Start.
  - **Pieces** (141 Level Piece actors: river, roads, bridges, fields, graveyards, trees, medical, dock, coal) are real
    meshes in the level with a material for each kind in `Content/Level` (`MI_Piece_Path`, `MI_Piece_River`, ...):
    a texture slot laid over the world so pieces tile as one surface, a tint, roughness. They still show their flat
    colors until textures are put in. Water has a wall of its own that stops the wizard. They are no longer cut
    away around the cursor.
  - **Houses**: all 111 house boxes are gone. 95 are house meshes (cottage, burned cottage, barn, two-story), each
    scaled toward the size of its box between 0.5 and 1.1 (most near 0.5: the boxes were small, so most doors are
    about 115 to 125 tall beside a wizard of 220); 16 boxes were removed where a house would have stood on a post's
    ground or the shrine. Each house mesh is solid by itself.
  - **Lamps**: all 20 are the lamp post model (`Content/Props/SM_Lamp_Post`, 320 tall), each turned its own way.
  - The layout actor has "Place Systems As Actors" and "Refresh Placed Actors" buttons. `Wiz.Goto <x> <y> | shrine`
    moves the wizard. `Wiz.State` has `placedPosts`.
- **The level's pieces are actors, and its ground is a landscape** (director, 2026-11-14 01:14 PST; built): every piece of the
  Outskirts (95 house boxes, 7 medical buildings, 49 trees, 41 stretches of river, 33 of road, 20 lamps, the bridges,
  fields, graveyards, dock and coal: 256 in all) is its own **Level Piece** actor in the level, in folders by kind
  under `Layout`. Each can be moved, turned, scaled, duplicated, deleted, or given another kind (`Kind`, `Size`). The
  game reads them when a run starts: houses, medical, dock, coal, trees and river block; the rest are ground. A turned
  blocker blocks the square box around it. The game still draws them in a few batches, cut away around the cursor,
  so the web build pays nothing for their number; the shapes seen in the editor are editor-only. "Use Placed Pieces"
  on the layout actor says the level's actors are the pieces; off, the code-made layout is used again.
  **16 house boxes are replaced by the house meshes** (cottage, burned cottage, barn, two-story house), where a mesh
  fit without touching a road, the river or another building; their doors face no particular way yet.
  **The ground is a landscape** ("Ground Landscape", 315 by 252 meters, one meter a quad) with four paint layers:
  Ground, Grass, Dirt, Stone. Each layer is a texture and a tint on `Content/Landscape/MI_Ground`; all four start as
  flat colors and the whole of it is painted Ground. It must stay flat: units walk a flat plane, so sculpting it
  would leave them floating or buried. **Still code**: where the wizard starts, the shrine, the posts, the commander
  and the patrols' roads. Moving a road piece does not move the patrol that walks it.
- **The Outskirts is a level in the project** (director, 2026-11-13 23:10 PST; built): `Content/Maps/Outskirts`, the map the game and the
  editor now start in. It is the engine's default template (floor, sky, light, player start) with one actor in it,
  "Outskirts Layout", which draws the river, roads, fields, trees and house boxes in the editor as the game lays them
  out. They are a picture to build against: they cannot be moved in the editor (the layout is still code). **A
  building placed in the level blocks**: any static mesh from `Content/Buildings` (or any static mesh actor with the
  actor tag `Blocker`) stops units, Bolts and the wizard, by a box 85% of its footprint. **"Show House Boxes"** on the
  layout actor (or `Wiz.HouseBoxes`) removes the plain house boxes, from the picture and from the way, once real
  buildings stand. Placed buildings are not cut away around the cursor yet, so a tall one can hide the fight.
- **Building textures** (director, 2026-11-13 23:10 PST: "kind of blurry"): color and normal maps now keep their full 2048 (they were cut
  to 1024), roughness and metal 1024, and the smaller copies of the color map are sharpened.
- **Two swords are imported, not yet held** (director, 2026-11-14 00:15 PST): `Content/Weapons/SM_Iron_Longsword` (120 long) and
  `SM_Steel_Sword` (95 long), each with one material and 512 textures. They are made to be held: the middle of the
  grip is the mesh's origin, the blade points up (+Z), the crossguard lies along X. Nothing in the game uses them yet.
- **Houses and the shrine are imported, not yet placed** (director, 2026-11-13 22:20 PST): `Content/Buildings/SM_Peasant_Cottage`,
  `SM_Burned_Cottage`, `SM_Barn`, `SM_Twostory_House`, `SM_Shrine`, each with its material and three 1024 textures.
  Each is scaled so its doorway is about 230 units tall beside the 220 tall wizard (the two cottages 240 and 221, as
  they share one scale), with the doorstep at height 0 and its slab under the ground. Sizes: cottages 1162 x 972 x 767,
  barn 1102 x 1503 x 711, two-story house 1205 x 1289 x 1087, shrine 881 x 881 x 701. The shrine is a ruined chapel
  with no doorway; it is sized so its window sill is at chest height (the agent's choice). The level still draws
  boxes; the folder is not cooked into the web build until something uses it.
- **The Guard and the grunt are models too** (director, 2026-11-13 17:30 PST; built): `Content/Army/SKM_Guard` at 175 units and
  `Content/Enemy/SKM_Grunt` at 170, through the same route as the skeleton (rig cleaned in Blender, bones turned to the
  mannequin's). For each kind nine hidden figures do the animating: standing, strolling, running, and six copies of
  the swing started a sixth of a second apart; a figure that strikes copies the swing whose blow lands with its own.
  At most 140 figures are drawn as models at once; enemies past that, the farthest from the wizard, are shapes.
- **The army's plain skeleton is a model** (director, 2026-11-13 15:40 PST; built): `Content/Army/SKM_Skeleton`, from the
  partner mesh service, on the mannequin's skeleton, moved by the mannequin's animation blueprint for now. It is
  imported at 66 units, the size agreed for a skeleton beside the wizard (0.3 of 220), and drawn at whatever
  `SkeletonSize` says. **Every character model imported from now on is made to the agreed size of its kind** (a test
  checks this one). Forty of them are cheap: three hidden figures run the animation (standing, strolling, running)
  and each skeleton on screen copies one, so all that stroll step together. **To change later:** bake the animations
  into the mesh, or give each its own, when they should differ. Guards are still shapes. `Wiz.ArmyModel 0` puts the
  shapes back.
- **The finished game has real character models** for the wizard, the army and the enemies. They replace the shapes
  once the game is fun with shapes. Art must stay within the browser's renderer: a crowd of 160 cannot be 160
  animated skeletons in a browser, so crowd models are low-polygon, seen from the Angled camera, and moved by code
  (bob, lean, turn) or baked vertex animation, not full skeletal animation. Only the wizard and the Tyrant may get more.
- HUD text is drawn from an outline font at the size it is shown, so it is sharp from 640x480 to 4K.
- Sound and music are a judging criterion and go in by day three at the latest.

**The wizard's robe (planned, if time permits; director, 2026-11-13 11:30 PST):** the robe is skinned as one stiff bell, so
at a full jog his legs kick out through its back. The fix is to re-skin the robe, not to retarget. Plan, time-boxed to
45 minutes, after the next gameplay build:
1. Measure (Blender, no window): print which bones the vertices below the pelvis follow. Expected: the pelvis alone.
2. Re-skin with a script (`Tools/Art/reskin_robe.py`, to be written): each robe vertex below the pelvis follows the
   pelvis at the waist and, more and more toward the hem, the thigh and calf of its own side; the front and back
   middle are shared between both legs; weights are smoothed. The robe then swings with each stride.
3. Re-import with `Tools/Art/import_mannequin_character.py` and take shots of a full jog from behind and from the
   side in the Third Person camera. Pass: no leg shows through the robe at any point of the stride.
4. Check the same in the web build.
If the re-skin does not pass in the time: keep the mesh and give him a gait that suits a robe instead (a shorter
stride or a glide when his own animations are imported). Not chosen: extra robe bones with cloth motion (best look,
but more bones than the mannequin skeleton has and more cost on the web).

### Animations, music and sound effects (director, 2026-11-14 22:30 PST; built)

- **Deaths**, for every troop of both sides: the figure falls where it stood, lies 2.5 s, then sinks into the ground
  over 1.5 s (slow at first). Which death is by where the last blow came from, as the unit stood: from the front
  (within 60 degrees of straight ahead) `A_Kallari_Death_B`; from behind `A_Narbash_Death_Fwd`; from a side
  `A_Grux_Death_B`. (Measured on the animations: none of the three falls backward. Narbash pitches forward, Kallari
  staggers back and then drops, Grux is thrown off his feet.) At most 24 lie at once.
- **Overwhelmed**: `A_Khaimera_Stunned_Start` comes in over 0.45 s, eased at both ends, from whatever the wizard was
  doing; he is on his knees by about 1.2 s and starts to go under at 60% of the fall.
- **Attacks**: skeletons, Guards and the enemy's soldiers take `A_Greystone_Attack_A_Med` or `A_Minions_Attack_B_SetB`,
  a new pick for each blow. Brute: `A_Terra_Primary_Melee_D_slow`. Assassin: `A_Kallari_Attack_GuardBreak_A`, while it
  is on the wizard. Commander: `A_Greystone_Attack_PrimaryA` for his slash, `A_Narbash_RMB_Ability_Throw` for his
  throw. A slam, a slash and a throw stretch the animation up to its blow over the wind-up and play the rest at
  its own speed. **Where the blow lands in each is measured, until the director marks it**: a notify named "Fire"
  on the animation, or a montage beside it named `AM_...` in place of `A_...` (the game takes the montage when there
  is one). A notify named "Fall" on a death marks when the body meets the ground.
- Kept and not yet used: `A_Greystone_Jog_Fwd`, `A_Gideon_LevelStart`. The other 63 picks are out of the project.
- **Music**: `MUS_01_Menu_Darkness_Rises` on the start menu (and the settings opened from it),
  `MUS_03_Forgotten_Pleasure` in the game, `MUS_02_Start_Beyond_The_Horizon` once the gate is entered; each loops.
  A change fades the old out and the new in over 2.5 s. The music carries on through a restart.
- **Sound effects** (each quieter the farther from the wizard, silent past 3200 units; one sound is not started
  twice within about a tenth of a second): Blink; Summon that raises something; a skeleton's blow on a soldier,
  archer or assassin (`Sword_Hits_Enemy`) and on a brute or the commander (`Sword_Hit_Armor`); a skeleton, Guard or
  Marksman dying (`Skeleton_Dies`, from 0.3 s in: its second rattle); a brute's slam on nobody
  (`Hammer_Hits_Ground`); the Bolt thrown (`Fire_Throw`) and meeting an enemy or a wall (`Fire_Impact`; not when it
  runs out of range); an assassin's swing that finds the wizard gone or mid-blink (`Daggar_Miss`, its first 0.5 s);
  an archer's or Marksman's shot (`Bow_Shoot`); a body meeting the ground, timed to its death, on dirt or grass by
  the landscape's paint (`Body_Fall_Dirt_01/02`, `Body_Fall_Grass_01/02`). Refused (a Blink on cooldown, a Summon with
  no bones, a spear that cannot be thrown, a purchase that buys nothing, a key that cannot be used): the interface
  click. **The director wrote `MUS_02` for this; taken as a slip for the click: to confirm.**
- The assassin's swing changes nothing in the numbers: being on the wizard is still what it does to him.
- The ground's paint cannot be read in a packaged game: `Tools/Level/bake_ground_paint.py` bakes it into the
  settings (dirt: the Ground, Dirt, DirtySoil and Stone layers). **Run it again after repainting.**
- Commands: `Wiz.Sound [Cue]`, `Wiz.Music [menu|game|end|none]`, `Wiz.Ground <x> <y>`, `Wiz.Floor <x0> <y0> <x1> <y1>`.

### Montages, weapons in hand, the brute's sweep (director, 2026-11-15 00:20 PST; built)

- **The director's montages are what the game plays**: for an animation `A_Name` the montage beside it is
  `Anim_A_Name`, and the game takes it when it is there. Their notifies are markers the game reads (they play no
  sound by themselves): `Attack` is the moment a blow lands, `ThrowSpear` the moment the spear leaves, `PlaySound`
  on a death the moment the body meets the ground. (`Fire` on the older montages means the same as `Attack`.)
  What `PlaySound` should play on the attack montages is to be gone through with the director, one by one.
- **The thud of a body is heard on the death's `PlaySound`**, not started there: each body-fall sound has quiet
  before its thud (dirt 0.545 and 0.54 s, grass 0.225 and 0.195 s, measured on the files), so it is started that
  much early. Deaths: Kallari 1.000 s, Narbash 0.886 s, Grux 0.597 s.
- **Weapons in hand**: skeleton and Guard the steel sword, the enemy's soldier the iron longsword, all on
  `Sword_Socket` (changed by the director, 2026-11-15 01:05 PST: **steel sword for skeleton and Guard, iron
  longsword for the enemy's soldier**); assassin the dagger on `Dagger_Socket`;
  brute the hammer on `Weapon_Socket`; commander his sword, and his spear on `Commander_Spear_Socket` only while he
  draws back to throw it (his sword is put up for the throw). **No socket named `Commander_Attack_Sword` is on the
  model**, so his sword is on `Weapon_Socket` until there is one. A dead figure keeps what it held.
- **The brute has two attacks.** The slam (its own animation, the circle) is the heavy one: at most one every
  `BruteSlamEvery` 6 s. Between slams it **sweeps**: `Anim_A_Terra_Primary_Melee_D_slow`, everything of the army
  within 250 of it and inside 120 degrees across its front (a third of a circle) takes 16, the wizard 0.15 of the
  meter; 0.5 s wind-up shown as a fan of red bars, 0.9 s rest. Numbers are the agent's first guess (`Wiz.Tune
  BruteSweep...`, `BruteSlamEvery 0` = slams only, as before).
- **Shadows no longer change with the cursor**: the see-through circle was also being cut out of shadows, where it
  landed in the wrong places. The materials now leave shadows whole (`Tools/Art/see_through_shadows.py`).
- The game no longer changes the editor's "quiet when another window is in front" setting while it runs in the
  editor.
- Commands: `Wiz.Weapons`; `Wiz.State` has `sweeps`, `sweepHits`.

### The Bolt of fire, marks on the ground, music at no volume, a smaller download (director, 2026-11-15 01:50 PST; built)

- **The Bolt is a ball of fire** (`/Game/Level/M_FireBolt`): white-hot in the middle, orange to red at its rim,
  cooler toward its tail, flickering, and it swells and shrinks nine times a second. Longer along its flight than
  across. A Niagara effect is to be added to it later (director).
- **Marks on the ground** (`/Game/Level/M_GroundMark`, on flat planes): a glowing rim, a soft fill and a moving
  pattern. The enemy's warnings are embers on dried blood, dim while they grow and burning for their last moment:
  a slam's circle (a turning ring of dashes), the commander's spear lane (chevrons running the way the spear will
  go), and **the brute's sweep, now one glowing slice of a circle** (it was nine bars). Where the wizard's spear
  lands is his own violet. Colors and speeds are in `CrowdRenderer.cpp` (`Slam...Color/Glow`, `SpearMark...`).
- Both materials are made by `Tools/Art/make_effect_materials.py` (delete the asset and run it to change one).
- **Music plays on when its volume is nothing**, so it is there when the slider comes back up.
- **The ending's padlock** fades in with its button (its hoop was drawn with lines, which do not fade).
- **A smaller download**: 47 textures are capped (`Tools/Art/cap_texture_sizes.py`): houses, trees, stones and the
  kingdom's pieces at 1024 (they were 2048), the engine's mannequin (loaded for its animations, never seen) at 256.
  The pictures inside the assets are whole: "Maximum Texture Size" back to 0 undoes it. Expected to take about
  20 MB off the 87 MB content file; **not measured until the next web build**.
- Command: `Wiz.Effects`.

### Three more sounds, a light fog, the gate's ground nearer the castle gate (director, 2026-11-15 02:28 PST; built)

- **Sounds** (the director: "That is the rest of the sounds for now"): `bonepickup` every time a pile of bones is
  picked up (the player's own, not placed); `gateopen` when the kingdom gate opens (heard at full loudness wherever
  the wizard stands, since it tells him the way is open); `commanderspearthrow` when the commander's spear leaves his
  hand (placed where he stands). Play one with `Wiz.Sound <name>`; `Wiz.Sound` prints the counts.
- **Fog**: the level's height fog sits at the ground with a low density (0.035, falloff 0.05, a dim blue): a faint
  haze that lifts the darkest shadows. `Tools/Level/gate_and_fog.py` (change `FOG_*` and run again).
- **The Kingdom Gate actor** (the ground that finishes the game) is at 18804, 2491, about 270 units in front of the
  director's castle gate doors. It is still its own actor, with its turn and size as they were.

### The Bolt through houses, the commander's spear through the army, its sound with it (director, 2026-11-15 02:45 PST; built)

- **The wizard's Bolt passes through houses and lamp posts** (they still block walking). Anything else that blocked
  it still does: trees and other pieces of a code-made layout, bridge rails, the shrine, actors tagged `Blocker`
  that are not houses (the castle walls, if tagged). Rule: `UCrowdSubsystem::BoltPassesMesh` (a mesh under
  `/Game/Buildings/`, or a lamp post).
- **The commander's spear hits every one of the army in its path**, each once, for `CommanderSpearDamage` (20: a
  plain skeleton dies, a Guard does not), and flies on. It still stops at the wizard and at walls and houses.
  `Wiz.Tune CommanderSpearDamage <n>`.
- **The spear's sound flies with the spear**: it is moved every frame to where the spear is, and stays where the
  spear ended. `Wiz.Sound` prints `followed` and `followedMoves`.
- **Fewer engine plugins in the web build**: 26 plugins that nothing in the game uses are left out of the Wasm
  platform only (`WebTesting.uproject`, `PlatformDenyList: Wasm`); the editor is unchanged.
  `Tools/Jam/plugin_use.py` lists what the content uses.

## Technical direction

- Unreal Engine 5.9 Preview, C++, web (Shipping) as the entry, Windows zip as the fallback.
- Army and enemy crowds are drawn as instanced shapes and updated by one manager, not one actor each.
- Everything a tester would change has a console command and a readable value; debug overlays can be switched off.
- Package for the web at the end of day one and after every major feature. Size is checked each time.

## Out of scope

- A home base, saved progress between runs, multiplayer.
- Phones and tablets are wanted **if time permits** (director, 2026-11-12 16:07 PST): on-screen controls for moving, aiming,
  the spell, the spear, the shrine and pause. Not started. Everything in the HUD is already sized by the screen.
- Third-person camera art and animation, unless that camera is chosen.
- More than one level (2026-11-14 10:39 PST): Inside the Walls, the Castle and the Tyrant.

## Schedule

| Day | Goal |
|---|---|
| 1 (Nov 12) | Shapes only: move, Bolt, enemies, raising skeletons, army follows and fights, Overwhelm, debug camera. First web package. Then the base feel: army roaming, enemies charging the wizard, obstacles and pathing, sharp HUD text. |
| 2 (Nov 13) | Bone spear with one tough enemy (Brute), then shrines and sacrifice (the spear is the first unlock), the ability and perk list, merging, elites, the three areas. Start generating character models in Tripo alongside. Test upload to the draft itch.io page. |
| 3 (Nov 14) | Tyrant, final cut screen, art pass, sound and music, opening scene if time allows. Feature freeze at end of day. |
| 4 (Nov 15) | Fixes, balance, gameplay video, dev log, submit at least three hours before 5:59 PM PST. |

## Tools and services

Claude Code does the code, engine work, builds, and the dev log. Other services are used for assets so they don't
draw on Claude usage. Every tool used is listed in the dev log for the submission's "Tools you used" field.

| Task | Service | Notes |
|---|---|---|
| Characters and props (3D) | Tripo | 3,000 credits, adjustable polygon count, auto-rigging. No card needed. Redeem by Nov 16. |
| Hero models (wizard, Tyrant) | Hyper3D Rodin | 208 credits, so save it for the few models that matter. Listing Rodin also enters the Hyper3D cash pool. Redeem by Nov 16. |
| Materials and textures | PBRgen | Tileable ground and wall materials. Valid until Nov 15. |
| Sound effects and music | WeLoveIndies | Ready-made, free for non-commercial use. |
| Generated audio and concept images | fal | $30 credits. Custom sounds or stings the library lacks; concept art. Claim by Nov 30. |
| Extra coding help | Cursor (SpaceXSI credits) | $50, lasts 7 days from claiming, so claim when needed. |
| Overflow for Claude | Claude API credits | $25, for use with an API key if the subscription limit is reached. Opens Nov 12. |

**How assets get made:** when the build reaches a step that needs a character, mesh, material, or sound, Claude names
the service to use for it at that moment, with a prompt to paste; Screeps runs the service and hands back the file;
Claude imports it.

Not planned: Meshy (needs a credit card and renews automatically), Rokoko and Reallusion (only useful if the game
gets animated humanoids), Diversion (git is already set up), NetApp (after the jam).

## Open questions
- **Where projectiles are fired from** (director's question, 2026-11-14 17:20 PST; the agent's recommendation, not
  decided): keep the simulation firing from the unit's place on the ground, as now, and make only the picture start
  at the weapon: a socket on each bow mesh ("Arrow_Socket", at the nocked arrow) and one in the wizard's casting
  hand, read when the shot leaves, with the drawn arrow or spear eased from there onto the simulation's path over
  its first tenth of a second. Hits, ranges and tests stay exactly as they are.

- The game's name.
- Summon the Afterlife: one skeleton a cast, or all the bones at once (`SummonMax`; all at once today). And its name.
- The spear's price in bones (`SpearBoneCost`, 3 today).
- The map: night or overcast dusk, and how much rain.
- The list of shrine abilities and perks (after the base gameplay feels right).
- The bone numbers: 2 bones a skeleton, 1 bone a grunt, 130 units to collect, 30 seconds before bones fade.
- Research prices and the rest of the list; the stats tab (armor, health, base damage).
- Post sizes, the wake distance and the patrol timing, after play-testing.
- The kingdom gate: its model, whether a shut gate should block as a wall, and whether the fall gets the final cut
  screen too.
- The Outskirts drawing: which way the wizard goes (today: bottom of the east road to the top right), and what the
  light blue lines beside the east road are (read as part of the walkway).
- Shrine timing: usable all round (leaning) or only after the boss.
- How a round is counted: number of assaults, number of enemies, and what the commander is.
- Aim and Bolt firing get refined when the other abilities and ways to attack are designed.
- Difficulty setting: in or out.
- Opening scene: played in the engine with shapes, or still pictures with text.


## Decisions log

- 2026-11-11 21:10 PST: Solo entry; friend credited for the idea.
- 2026-11-11 21:10 PST: One linear run, no home base.
- 2026-11-11 21:20 PST: The god falls by being overwhelmed, not by a health bar. Hubris and warded-ground ideas rejected: there
  must always be a danger of falling.
- 2026-11-11 21:20 PST: Godbane elites can reach the wizard through any army.
- 2026-11-11 21:20 PST: Hard draw limit with merging into bigger skeletons.
- 2026-11-11 21:20 PST: First build uses simple shapes only.
- 2026-11-11 21:20 PST: Final cut screen on a fall and on completing a run.
- 2026-11-11 21:31 PST: Overwhelm must show its direction, by a screen effect that leans in from that side and a sound.
- 2026-11-11 21:31 PST: Game name and ability name left open; "Dust to Dust" is a placeholder for the ability only.
- 2026-11-11 21:31 PST: Asset services are chosen step by step: Claude names the service when an asset is needed.
- 2026-11-11 21:31 PST: Every dated log entry carries a time as well.
- 2026-11-11 22:06 PST: Day one core built in shapes. Debug keys 1, 2, 3 (camera) and H (overlay) added because a
  browser build has no console. First numbers, all open to playtest: Bolt kills a grunt in one hit, five Bolts a
  second; a skeleton beats a grunt one on one; each grunt on the wizard fills Overwhelm by 8% a second.
- 2026-11-11 22:12 PST: Opening scene wanted (the tyrant drives the wizard out), if time and size allow. Planned for
  day three, in the engine, skippable; first thing cut if the day runs late.
- 2026-11-12 02:00 PST: Camera is Angled. The other cameras stay for development builds only.
- 2026-11-12 02:00 PST: The army stays near the wizard and roams with him; it attacks only what comes close. Enemies
  charge the wizard first and fight a skeleton only when it is in their way.
- 2026-11-12 02:00 PST: The detonate ability becomes the bone spear: costs skeletons, thrown, explodes in an area,
  strong against weak hordes and weak against tough enemies. Built next round with a Brute; likely a shrine unlock.
- 2026-11-12 02:00 PST: Obstacles block the wizard, units and the Bolt; enemies path around them. Practice layout now,
  zone layouts later.
- 2026-11-12 02:00 PST: Shapes are for testing only. The finished game has real character models.
- 2026-11-12 02:00 PST: More shrine abilities and perks are designed after the base gameplay feels right.
- 2026-11-12 11:40 PST: The army stands in groups, like walls protecting the wizard; nobody stands alone.
- 2026-11-12 11:40 PST: Grunts (red cubes) stay as the weak opening enemy. How the army is created is being rethought.
- 2026-11-12 11:40 PST: Frame rate confirmed by the director: 120 fps at 4K in Brave. HUD text approved.
- 2026-11-12 12:15 PST: The army stands in 3 to 6 groups by its size, never more than 15 in one.
- 2026-11-12 12:15 PST: A kill leaves bones; the wizard must walk near to collect them; 3 bones assemble a skeleton.
  Bones will also pay for abilities, and tougher enemies will give better skeletons. There will be several kinds of
  enemy and several kinds of skeleton.
- 2026-11-12 12:15 PST: Enemies show a health bar only for a few seconds after taking damage.
- 2026-11-12 14:55 PST: The bone spear and the Brute are built. The spear costs 3 skeletons, as the wizard grabs
  them; the grab-and-dust animation comes later. A Brute's bones assemble a Guard, the first stronger skeleton.
- 2026-11-12 14:55 PST: Shrine interact moves to F, because E also throws the spear.
- 2026-11-12 14:55 PST: The map is a dark medieval land: dark rock, dead trees, fire, rain. Plan in "The map".
- 2026-11-12 14:55 PST: The itch.io test upload of the day two build loaded and played the same as the local build.
- 2026-11-12 15:25 PST: The spear is named the Spear of Sacrifice.
- 2026-11-12 15:25 PST: Every enemy leaves its own number of bones (grunt 1, Brute 3). A Brute no longer makes a
  Guard by itself; how bones become the different kinds of skeleton is being rethought.
- 2026-11-12 15:25 PST: Not an endless wave game: rounds with a set number of enemies and a commander at the end.
- 2026-11-12 15:25 PST: The shrine is built with a placeholder menu: the game runs while it is open, the wizard can
  walk but not use abilities, and it closes on Esc, a click outside, or walking away. A basic pause menu is in.
- 2026-11-12 16:07 PST: Bones no longer assemble by themselves. The starting spell Summon the Afterlife (Q or Space) spends
  them on plain skeletons; one cast or all at once is tunable and to be play-tested.
- 2026-11-12 16:07 PST: The Spear of Sacrifice is locked at the start and bought at the shrine.
- 2026-11-12 16:07 PST: The shrine is the place to upgrade (unlocks, stats, abilities, better units), paid in bones, with
  bulk buying. Small skeletons are summoned; better ones are bought. First list: spear, Guard, bone armor.
- 2026-11-12 16:07 PST: The shrine's circle is half again as big (345 units).
- 2026-11-12 16:07 PST: The game is the wizard charging into his old kingdom. Enemies hold their ground and do not chase
  him by default; he progresses through three areas: the Outskirts, Inside the Walls, the Castle to the throne room.
- 2026-11-12 16:07 PST: Lighting is pushed as far as the browser allows (baked lighting, shadows). Phones if time permits.
- 2026-11-12 20:15 PST: Shrine upgrades are research, bought once (armor, weapons, stats, abilities). Stepwise armor,
  health and base damage are kept apart, for a second tab later.
- 2026-11-12 20:15 PST: Time runs at half speed while the shrine's menu is open, instead of the fight going on at full speed.
- 2026-11-12 20:15 PST: The HUD does not list an ability that cannot be used.
- 2026-11-12 20:15 PST: Posts, patrols and the commander are built as proposed, to be play-tested.
- 2026-11-12 20:15 PST: Levels: the director draws in 2D, Claude blocks out, the director dresses with meshes. The
  Outskirts are blocked out from the first drawing.
- 2026-11-13 00:55 PST: Bolt has a maximum distance (850 units, under the 900 a post notices at), to end sniping.
- 2026-11-13 00:55 PST: A kill alerts the dead enemy's group (bug: a one-hit kill used to leave the rest standing).
- 2026-11-13 00:55 PST: Enemies at posts move about; road patrols walk the streets and are replaced; hunting parties every 25 s.
- 2026-11-13 00:55 PST: More enemies: 14 posts and 187 enemies in the Outskirts, cap 320.
- 2026-11-13 00:55 PST: The Outskirts are 1.5 times larger beside the wizard (15 units a pixel), houses 420 tall.
- 2026-11-13 00:55 PST: A movable camera in the angled view and a 3D view are being considered, after the scale is settled.
- 2026-11-13 10:25 PST: The wizard is the Wizard King model (Tripo, version 2) on the UE5 mannequin skeleton with the mannequin's default animation blueprint, 220 units tall.
- 2026-11-13 11:09 PST: The wizard is a Character (director: "it will pay off in the long run"), so mannequin animation blueprints read his movement themselves. The model got the mannequin's helper bones, which its legs needed to move.
- 2026-11-13 11:30 PST: Unit sizes are shares of the wizard's height: low tier 0.3, Guard 0.42, Brute 0.5, commander 1.2 (taller than him).
- 2026-11-13 11:30 PST: Blink (Space): a 0.5 second dash toward the mouse, up to 700 units, invulnerable, 3 second cooldown after it ends.
- 2026-11-13 11:30 PST: One key for each control: right mouse Spear, Q Summon, Space Blink, E shrine, P pause, F kept for commanding the army. Escape is unused.
- 2026-11-13 11:30 PST: The debug stats (H) and the camera keys (1, 2, 3) are in every build, hidden at the start of the shipped one.
- 2026-11-13 11:30 PST: Planned for the next build: archers, assassins, and commanding the army (F). Built in two rounds on the agent's advice, to have a playable build sooner.
- 2026-11-13 11:30 PST: The robe will be re-skinned if time permits (plan under Look and sound); no retarget.
- 2026-11-13 12:20 PST: Blink goes the way the wizard is moving (not toward the mouse) and a direction held during it bends it.
- 2026-11-13 12:20 PST: Archers are in: weaker than a grunt, arrows hit harder, they run when charged. One in five of a post's grunts.
- 2026-11-13 12:20 PST: Assassins are in: fast, frail, cloaked until 450 units from the wizard, ignore and go around the army. One every 40 seconds while the commander lives.
- 2026-11-13 12:20 PST: Command mode is in (F; wheel for the number, right mouse for the kind, left mouse sends). Sent units rejoin when the wizard comes within 300 units of their place. Recall is C.
- 2026-11-13 14:40 PST: Nothing walks on water, enemy or army.
- 2026-11-13 14:40 PST: Sent units spread over a circle sized to the group, move about in it, go around walls and water, and send two fifths after an enemy that shoots them.
- 2026-11-13 14:40 PST: Enemies fight the army units near them when the wizard is more than 700 units away; nearer, they go for him as before.
- 2026-11-13 14:40 PST: The assassin shows from 800 units and is whole at 300; the army sees it at 450.
- 2026-11-13 14:40 PST: Command mode shows the number right of the cursor and the kind under it; a kind the army lacks cannot be chosen.
- 2026-11-13 14:40 PST: Blink is a burst (fast start, slows to a stop) of 0.65 seconds, after the Nexus Dash, and steers 0.6 of its range.
- 2026-11-13 14:40 PST: Bolt fires every 0.45 seconds (was 0.2).
- 2026-11-13 14:40 PST: The army counts toward bone worth at a third of its cost; paying at the shrine with army units is planned.
- 2026-11-13 14:40 PST: Planned later: an attack roots the attacker; telegraphed move-sets for higher-tier enemies.
- 2026-11-13 15:40 PST: Army strength: option 1. Within 600 units of the wizard the army is at full strength; beyond, half damage dealt and half again as much taken.
- 2026-11-13 15:40 PST: Blink steers most at its start, and the wizard is not drawn until seven tenths of its time have passed.
- 2026-11-13 15:40 PST: A fallen ally leaves a third of its cost in bones, a whole number (skeleton 1, Guard 3).
- 2026-11-13 15:40 PST: Archers aim ahead of a moving target and are off by up to 4 degrees.
- 2026-11-13 15:40 PST: The army does not shove its own, and stands on open ground when the wizard stops at a wall or the river.
- 2026-11-13 15:40 PST: Bones on screen do not fade; their 30 seconds start when they leave the screen.
- 2026-11-13 15:40 PST: The army's plain skeleton is the partner model at the agreed size (66 units). All later character models keep to the agreed size.
- 2026-11-13 15:40 PST: At the shrine, bones are spent first and the army pays the rest; units spent so leave no bones (planned).
- 2026-11-13 15:40 PST: Planned: kinds of ally give the wizard bonuses by their number (Guards: more Overwhelm he can take).
- 2026-11-13 16:25 PST: Sizes follow the director's table: wizard 220, skeleton 150, Guard 175, grunt 170, brute 240. Archer and assassin 165, commander 286 (the agent's choices).
- 2026-11-13 16:25 PST: Units stand 70 apart, groups 280 out with 100 of shuffle, roam circle 420, to fit the larger figures.
- 2026-11-13 16:25 PST: Melee is one blow a second for a second's worth of damage; enemies on the wizard still fill Overwhelm steadily.
- 2026-11-13 16:25 PST: The army pays at the shrine after the bones (skeleton 1, Guard 3), one purchase at a time, and leaves no bones.
- 2026-11-13 16:25 PST: Buildings, walls and trees are cut away in a circle around the cursor.
- 2026-11-13 16:25 PST: Army bonuses to the wizard will follow curves with falling returns (Guards proposed: 20% at 4, 28% at 8, cap 35%).
- 2026-11-13 16:35 PST: The tyrant is the good king; the player is the villain and only the wizard calls him a tyrant. Enemies are the kingdom's honorable defenders (relayed by the assets session).
- 2026-11-13 17:30 PST: The spear is paid in bones (4) when he has them; otherwise 3 plain skeletons with him are cut down. No other kind is ever taken.
- 2026-11-13 17:30 PST: Units take the room of their bodies (a quarter of their height); reach, the wizard's body and the touch range follow. Enemies at posts no longer stand in each other.
- 2026-11-13 17:30 PST: A brute takes half from the front, double from the back, and turns at 140 degrees a second.
- 2026-11-13 17:30 PST: Guard (175) and grunt (170) models are in; units with a model swing as they strike, timed to the blow (temporary animation).
- 2026-11-13 17:30 PST: Army bonus curves are edited in a page of their own; not in the game yet.
- 2026-11-13 18:45 PST: The engine's GPU skin cache is off (`r.SkinCache.CompileShaders=False`, `r.SkinCache.Mode=0`). In the browser it made every model on screen create a new buffer and bind group each frame, and frames dropped about every 1.4 seconds. A test keeps it off.
- 2026-11-13 22:20 PST: The brute slams: a red circle grows in front of it, and 0.3 seconds after it is full the slam does 40 to the army inside and 35% of the meter to the wizard.
- 2026-11-13 22:20 PST: Every melee unit stands and does not turn while it attacks; a blow already started stays wound up while its target runs.
- 2026-11-13 22:20 PST: A skeleton costs 2 bones to summon (was 3); the spear 3 bones (was 4).
- 2026-11-13 22:20 PST: The assassin model is in (165). The archer's file has no rig. Four houses and the shrine are imported at door scale (230), not placed.
- 2026-11-13 22:20 PST: Striking figures stood in their bind pose because the swing animation was given before the figure was registered; fixed.
- 2026-11-13 23:10 PST: The Outskirts is a real level (`Content/Maps/Outskirts`) with a layout actor that shows the code-made layout in the editor; placed buildings block; house boxes can be switched off.
- 2026-11-13 23:10 PST: Archer (165) and brute (240) models are in. Building color and normal textures are 2048.
- 2026-11-14 00:15 PST: One slam leaves a guard at half health and, with bone armor researched, a skeleton at a third. Two swords imported (`Content/Weapons`), not yet held by anyone.
- 2026-11-14 00:52 PST: Enemies from posts and patrols lose the wizard by distance or around a corner: to where he was last seen, 3 seconds of looking, then back to where they stood. Written, not built yet.
- 2026-11-14 01:14 PST: The Outskirts' pieces are Level Piece actors in the level (movable, deletable); 16 house boxes are house meshes; the ground is a flat landscape with four paint layers. Posts, patrol roads, shrine and start stay in code.
- 2026-11-14 02:23 PST: The Outskirts is laid out in the level: shrine, posts, commander, patrol roads and start are actors; pieces are real meshes with a material each; all house boxes are meshes or gone; lamps are the lamp model. Nothing is laid out at run time.
- 2026-11-14 10:39 PST: The game is one level, the Outskirts. Inside the Walls, the Castle and the Tyrant are cut for time; the commander is the last enemy. The hours go to sound, effects and polish.
- 2026-11-14 10:42 PST: The game finishes when the wizard enters the kingdom gates. Not built yet.
- 2026-11-14 10:49 PST: The kingdom gate opens only once the commander is dead; entering it shows the final cut screen. Written as a Kingdom Gate actor, not built or placed yet.
- 2026-11-14 11:19 PST: The commander slashes (a quick slam) and throws a spear that slows; he always goes for the wizard and weighs most on Overwhelm, shown by a HUD icon. Overwhelm waits 2 seconds before it goes down. The wizard's spear costs one skeleton. The Marksman is the last friendly unit (shrine, 6 bones: agent's choice). Arrows arc. Written, not built.
- 2026-11-14 11:58 PST: The commander's spear throw has a warning lane that locks before the throw. The HUD is the UI thread's round 2 design with the director's icons. Everything since the kingdom gate is now built and tested.
- 2026-11-14 14:16 PST: The Outskirts is lit as night (a dim blue moon, fixed exposure). The start menu, the pause menu with clickable buttons and the controls panel, and the shrine's new panel are built from the UI thread's menus round 2.
- 2026-11-14 14:31 PST: Titles and labels are Cinzel Bold, downloaded with the director's OK (SIL Open Font License).
- 2026-11-14 15:15 PST: The final cut screen (tally, ledger of kept / cut / fell, "March again", a locked "Enter the kingdom") and the commander-has-fallen banner with the pointer to the gate are built from the UI thread's designs. The gate message has no bars (design version 4).
- 2026-11-14 18:07 PST: The commander predicts half his throws. The brute's weak point mark is gone. Pointer on menus, crosshair in the game. Attacks are timed by the animations' "Fire" notifies; the Bolt cast is on the upper body and leaves 0.2 s after the cast starts. Bows are held. Arrows, the wizard's spear and bone piles are models. The river is one smooth mesh with moving water; walls are actors; two Megascans ground layers can be painted.
- 2026-11-14 19:04 PST: The Bolt's throwing arm points ahead whatever the legs do. Road blocks are removed (roads are painted). Bridges are the bridge model, over lower and more lifelike water. The brute holds its hammer; the commander throws his own spear model; the Marksman has its shrine picture. Turned houses block only their walls.
- 2026-11-14 20:28 PST: The game is called The Bone King (relayed). The walls are replaced by a battlefield edge that warns, then fails the mission. Marksmen stand to shoot and keep away. Only plain skeletons are sacrificed or counted. Houses and trees are cut away around the cursor; walls are not. Blocking comes from the collision made in the mesh editor; the crowd stands on the bridge's deck. The web package now includes the animations, weapons and props.
- 2026-11-14 21:40 PST: The settings screen is built from the UI thread's hand-off (Controls read-only, Sound shown but not usable, Display working; saved between runs). The river's mesh is put back to the one from before the last run of its script, which zigzagged at the bridges. The kingdom's wall, gate, gate door and tower models are imported, not placed. The Paragon animation set was surveyed and picks copied to a folder for the director; none imported.
- 2026-11-14 22:12 PST: Keys can be changed on the Controls tab (twelve controls; two swap; saved). Sound has four channels (master, music, effects, interface) that the Sound tab's volumes and mutes drive; the director's 21 sounds are imported onto them, not yet played by the game. The 75 Paragon picks are imported into /Game/Paragon_Picks to be looked at; none is used by the game.
- 2026-11-14 23:40 PST: The chosen Paragon animations are in the game (deaths by where the blow came from, the stun, two sword blows picked per blow, the brute's, the assassin's, the commander's slash and throw); the other 63 picks are removed. Music plays for the menu, the game and the end with 2.5 s fades. Fifteen sound effects are wired. The bridge model has collision made for walking (ramps and two rails). The trees' leaves are darker. The refused-action sound is the interface click (the direction named MUS_02: to confirm).
- 2026-11-15 00:53 PST: The director's montages (Anim_ before the name) are what the game plays, and their notifies time the blows, the throw and the body falls; a body's thud is heard on the notify (started early by the sound's quiet lead-in). Weapons are in hand on the director's sockets. The brute sweeps a third of a circle between slams. The see-through circle no longer cuts shadows.
- 2026-11-15 01:20 PST: A dead figure holds the last frame of its death (it stood up in its bind pose when the montage ended). Sounds from outside the player are played at their place in 3D, heard from where the wizard stands. The volume sliders are applied by the game to every sound it starts, not through an engine sound mix (which did nothing in the editor).
- 2026-11-15 01:41 PST: Swords: steel for skeleton and Guard, iron longsword for the enemy's soldier (director). The web page is given `-project=` in its arguments (`PageArgs` in `Config/DefaultEngine.ini`): without it the browser kept a saved folder that is not the game's and settings were lost on reload. Web build of this date: 274 MB staged (58 files); 238 MB without the compressed copies, which is what an itch.io zip would hold; largest file the .wasm at 133 MB; the browser downloaded 125 MB. Under itch.io's 500 MB total and 200 MB per file..
- 2026-11-15 02:09 PST: The Bolt is a pulsing ball of fire. The enemy's warnings and the spear's landing are glowing marks on the ground (embers for the enemy, violet for the wizard); the brute's sweep is one slice of a circle. Music plays on at no volume. The ending's padlock fades in with its button. 47 textures capped at 1024 (mannequin 256) for a smaller download.
- 2026-11-15 02:39 PST: Three more sounds: a bone pile picked up, the gate opening (heard anywhere), the commander's throw (from where he stands). A faint fog over the map. The Kingdom Gate actor moved nearer the director's castle gate, still its own actor.
- 2026-11-15 03:00 PST: The Bolt passes through houses and lamp posts. The commander's spear hits each of the army in its path once (20) and flies on; its sound flies with it. 26 unused engine plugins left out of the web build.
