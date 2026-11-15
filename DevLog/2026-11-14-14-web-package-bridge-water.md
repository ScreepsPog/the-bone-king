# Web package with the bridge, water, models and attack animations (2026-11-14 19:23 PST)

## When

2026-11-14 19:04 PST to 19:23 PST.

## Asked

The director wrote: "Then build a playable web version, no a itch package yet"

## What the agent did

Claude Code (Claude Fable 5.2).

- First package **failed** after 38 seconds, at the link step of the web build: three level functions made in the
  last two rounds (place walls, refresh placed pieces, tighten building collision) exist only in the editor but were
  declared for the game too. Declared editor-only (commit after `46eb019`), compiled, unit tests rerun.
- Second package: **succeeded** in 13 minutes 55 seconds, 265 MB in `Saved/WebBuild/Wasm`. No itch.io zip.
- Opened at http://localhost:8900/ in the app's browser pane.

## Tests

- Unit tests after the fix: 189 passed, 1 failed (the lamp post by the shrine, the director's placement).
- In the browser: the page loads (level in 49.8 s, with shaders prepared), the start menu shows with the Cinzel
  title, and Enter starts the run on the night level with the painted ground.
- The browser pane was not on screen, so the page ran at about 1 frame a second ("measured 1.0" in its own log).
  At that rate a quick click on March falls between frames and does nothing; a held Enter started the run. This is
  the hidden pane, not the game: the earlier package, opened while the pane ran at full speed, took the click.

## Build

Shipping web package: succeeded on the second try.

## Not done, not checked

- Nothing was judged for look or feel in the browser at 1 frame a second: not the water, bridge, bows, hammer, cast
  animation, frame rate or the click on March at full speed. The director should play it with the pane or a
  browser tab on screen.
- No itch.io zip; nothing uploaded.

## Tools used

Claude Code (Claude Fable 5.2); Unreal Engine 5.9 Preview (UAT BuildCookRun, Wasm); the Claude desktop app's browser.
