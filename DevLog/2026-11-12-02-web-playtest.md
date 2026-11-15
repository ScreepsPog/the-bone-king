# Web playtest by the agent (2026-11-12 09:55 PST to 10:05 PST)

## When

2026-11-12 09:55 PST to 10:05 PST.

## Asked

The director approved one agent playtest of the web build ("you can test it now").

## What the agents did

Claude Code (Claude Fable 5.2) played the Development web build from 02:23 PST at `localhost:8900/?tier=forward`.

- **Where.** In the Claude app's browser pane (Chromium, the same engine as Brave), with the pane on screen. Computer
  use can look at Brave but may not click or type in it, so Brave itself was not driven.
- **How.** Keys and the mouse were held down by a script in the page (W, A, S, D in turn every 2.5 s, left mouse held
  while the aim swept in a circle). The game took this input like real input.
- **Length.** One run of 139 s: 84 s moving and firing, then 55 s standing still without firing.

## Results

| Check | Result |
|---|---|
| Frame rate | 118 to 121 fps for the whole run, with up to 63 units drawn, at 783x1719 (display scaled 150%). |
| Move, aim, hold to fire | Work. The wizard stops at walls. Bolts kill and raise skeletons. |
| Sharp text | Sharp at real display pixels. |
| Browser errors | None. |

## What the playtest found

1. **No danger.** The army reached its cap of 40 in about 40 s and Overwhelm stayed at 0% for the whole run. Standing
   still without firing for 55 s, the army alone raised kills from about 150 to 250. The worry from last round (the
   army is a weaker shield) was wrong at the current wave size: about 20 grunts at a time cannot get through 40
   skeletons.
2. **The army clumps.** Skeletons bunch into two or three tight blocks, often on or past the edge of the roam circle,
   instead of spreading through it. While the wizard walks, the whole army trails as one packed block behind him.
   See `Images/2026-11-12-playtest-army-clumps.jpg`.
3. **A banner covers the HUD.** The engine's page shows "plays best in Chrome" over the Army count and the Overwhelm
   bar until it is closed. It will show in Brave too.
4. **The run restarted once in the first seconds** (the run clock went from 4 s back to 3 s) with Overwhelm at 0%.
   Not reproduced, cause unknown.

## Not checked

- Brave itself, and 4K full screen (the pane is 783x1719, about one sixth of the pixels of 4K).
- How it feels in the hand: a script held the keys.
- The 03:00 PST scheduled playtest started at 03:25 PST and left no report.

## Tools used

Claude Code with Claude Fable 5.2, the Claude app's browser pane.

## Time spent

About 10 minutes.
