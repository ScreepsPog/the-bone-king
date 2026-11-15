# Enemies lose sight of the wizard, search, and go back (2026-11-14 00:36 PST to 00:53 PST)

## When

2026-11-14 00:36 PST to 00:53 PST.

## Asked

The director wrote: "make it to where when AI lose sight of you, whether that be a distance or corner, they go to last
seen spot, look for a few seconds, the run back to where they originally were when it started. Do not build yet, just
do those things".

## What the agent did

Claude Code (Claude Fable 5.2). Code and tests were written. Nothing was built or run, as asked.

- An enemy from a post or a road patrol sees the wizard within 1300 units when no wall or building is between them.
- A group shares what it sees: while one of it sees him, all of it chases.
- When none sees him, each goes to where he was last seen, looks about for 3 seconds, runs back to where it stood
  when it noticed him, and takes up its post or patrol again.
- A wall now also keeps a group from noticing him to begin with. Otherwise a group behind a house would be roused,
  search, go back, and be roused again without end.
- Waves, raids, assassins and groups woken by `WakeAll` still always know where he is.

## Tests

- Five new unit tests in `Source/WebTesting/Tests/LostSightTests.cpp`: lost by distance, lost around a corner, seen
  again, a group shares sight, sent enemies never lose him.
- **None has been compiled or run.** They could not be run against the old code first either.
- Older garrison and patrol tests may fail on the next build where they assumed a roused group chases forever or is
  roused through a wall. Those are to be looked at one by one then, not weakened.

## Not done, not checked

- Not built, not tested, not packaged. The web build at `http://localhost:8900/` is the one from before this change.
- No scenario. No on-screen sign that an enemy is searching.
- The sight distance (1300), the give-up times and the arrival distance are guesses.

## Tools used

Claude Code (Claude Fable 5.2). No engine tools were run.
