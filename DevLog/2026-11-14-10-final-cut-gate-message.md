# The final cut screen and the gate message (2026-11-14 14:50 PST to 15:15 PST)

## When

2026-11-14 14:50 PST to 15:15 PST.

## Asked

The director wrote: "Build the final cut screen and the kingdom is open message."

While it was being built, the UI thread sent the director's latest change to the gate message (design version 4):
no cinematic bars, the banner higher (just under the Overwhelm bar), new timings.

## What the agent did

Claude Code (Claude Fable 5.2).

- **Final cut screen**, from `final-cut-handoff.md`: the parts come in one after another; the gold title "The
  Outskirts are yours"; the time; four numbers that count up; the ledger bar (kept, cut for power, fell in
  battle); "March again" (R or a click); a locked "Enter the kingdom" with hover text.
- **Gate message**, from `gate-handoff.md`: first built as version 3 (with bars), then changed to version 4 when it
  arrived: the banner with the broken crown, then the "Enter the kingdom gate" line with the distance, and a
  pointer (an arrow at the screen's edge, or a mark over the gate when it is in view).
- New tester command `Wiz.TakeArea`: plays the message and opens the gate without killing the commander.

## Tests

- New unit tests (5): the ledger's three parts (and never below zero, and no division by zero); the parts of the
  screen come in in order and the numbers count up; the two buttons at four screen sizes; the banner's times and
  that it never shows together with the line that follows; where the edge arrow goes.
- **Tests changed**, both mine from this round: the arrow's distance from the top (150 in the design) is 200,
  because at 150 it sat on the "Enter the kingdom gate" line in the first screenshot; and the timing test was
  rewritten for version 4 (no bars).
- Unit tests: 180 passed, 0 failed.
- Scenarios: 23 passed, 0 failed on the last full run. In the full run before it `day_six_orders` failed once; it
  then passed three runs alone and the next full run. The cause was not found (its log had been overwritten), so
  it may fail again.
- New scenario `day_fourteen_final_cut`: the area is taken, the banner shows, the gate opens, the wizard goes to
  the gate and the game ends.
- Screenshots looked at (`DevLog/Images/2026-11-14-10-*.png`): the banner; the objective line with the arrow; the
  final cut screen.

## Build

Editor build: succeeded. No web package was made.

## Not done, not checked

- Not played by hand: the hover text, the click on "March again", and the mark over the gate when it is in view
  were not seen (the screenshots show the arrow, not the mark).
- The final cut screenshot is from before the last two small changes (label spacing, arrow distance); it was
  retaken after them but not looked at again.
- The broken crown is small and dark; it reads as a red shape with a line through it more than as a crown.
- Left out of the final cut: the title's glow and the darker corners. The arrow is two strokes, not a filled head.
- Not seen in a browser.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview. Designs from the "UI
through Artifacts" thread (Claude Code).
