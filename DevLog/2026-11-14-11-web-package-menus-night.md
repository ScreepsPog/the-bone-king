# Web package with the night, the menus and the final cut (2026-11-14 16:25 PST)

## When

2026-11-14 16:08 PST to 16:25 PST.

## Asked

The agent asked whether to package the web build. The director answered: "Yes, package"

## What the agent did

Claude Code (Claude Fable 5.2).

- Packaged the Shipping web build of commit `ffda168` (BuildCookRun, Wasm): succeeded in 12 minutes 10 seconds,
  246 MB in `Saved/WebBuild/Wasm`. No zip for itch.io was made (not asked for).
- Opened it at http://localhost:8900/ in the app's browser pane, 800 by 600.

## Tests

Seen in the browser:
- The start menu shows first, with the picture, Cinzel titles and both buttons.
- A click on "March" starts the run.
- The level is night: dark blue ground, the wizard and houses readable. It is a little brighter and bluer than in
  the editor's screenshots.
- P shows the pause menu with the controls panel and icons; P again resumes.
- First picture came 30 seconds after opening the page (the pane was not on screen, which slows a page down).

## Build

Shipping web package: succeeded.

## Not done, not checked

- Not checked in the browser: the shrine menu, the gate message, the final cut screen, a fight, the frame rate.
- In an 800 by 600 window the longest control hint ("Wheel: how many ...") runs a few pixels over the controls
  panel's right edge. It fits at 1280 by 720 and larger.
- No itch.io zip; nothing uploaded.

## Tools used

Claude Code (Claude Fable 5.2); Unreal Engine 5.9 Preview (UAT BuildCookRun, Wasm); the Claude desktop app's browser.
