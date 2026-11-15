# Cinzel for the titles (2026-11-14 14:17 PST to 14:31 PST)

## When

2026-11-14 14:17 PST to 14:31 PST.

## Asked

The agent asked whether it may download the Cinzel font from Google Fonts for the titles. The director answered:
"Yes, download"

## What the agent did

Claude Code (Claude Fable 5.2).

- Downloaded `Cinzel-Bold.ttf` and `OFL.txt` (SIL Open Font License 1.1) from the font's own repository,
  https://github.com/NDISCOVER/Cinzel, into `Tools/Art/Fonts/Cinzel`. That is the source Google Fonts publishes from;
  it was used because it has a plain Bold file. A Medium file does not exist there, so the one line the design has in
  Medium is Bold.
- Imported it as `/Game/UI/Fonts/FF_Cinzel_Bold`, stored inside its asset so a package needs no loose file.
- Titles and labels are now Cinzel Bold with the design's letter spacing: "ARMY", "OVERWHELM", "SHRINE", "RESEARCH",
  "PAUSED", "CONTROLS", the start menu's title and the line over it. Everything else stays Roboto.
- First try drew nothing: a canvas text needs a font object, not only a typeface. Fixed by making one from the face.

## Tests

- New unit test `Hud.TheTitleFontIsCinzelBold`: the face is in the project and stored inside its asset. It failed
  before the import ("Expected 'the title font' to be not null"). **Test changed before it ever passed**: it first
  asked for a font asset; it now asks for the font face, because the font is made from the face in code.
- Unit tests: 175 passed, 0 failed. Scenarios: 22 passed, 0 failed.
- Screenshots looked at (`DevLog/Images/2026-11-14-09-*.png`): start menu, pause menu, controls, shrine.

## Build

Editor build: succeeded. No web package was made.

## Not done, not checked

- Not seen in a browser: whether the font draws in the web build is unproven until it is packaged.
- Not played by hand.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview. Font: Cinzel (SIL OFL).
