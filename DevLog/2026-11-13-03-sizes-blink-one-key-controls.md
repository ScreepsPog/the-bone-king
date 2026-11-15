# Unit sizes, Blink, one key per control (2026-11-13 11:13 PST to 11:47 PST)

## When

2026-11-13 11:13 PST to 11:47 PST.

## Asked

The director wrote (the parts built in this round):

- "Change the size of the low-tier guys to be smaller in scale to the player, this includes both friendly and enemy AI.
  For brutes, they should be slightly bigger but still smaller then the player. Commanders/bosses should be similar or
  even bigger."
- "Also in the next build, have the debug stats and camera that I can test. Also change the controls so that they only
  have 1 button assigned to them"
- "Add the ability to blink (Space keybind). By blink, I mean a dash but you become invulnerable during that dash...
  Make this take .5s to complete with a cooldown once completed. Have a max range you can blink and it goes in the
  direction of the mouse location."
- "Plan a reskin of the robe so it doesn't clip if time is permitted."
- "As for the batch sizes, go with your recommendation, as well with the keybinds."

Left for the next build, on the agent's advice (two rounds of testing instead of one): archers, assassins, and
commanding the army with F. Their direction is written into the design document.

## What the agent did

Claude Code (Claude Fable 5.2).

- **Sizes.** Every unit's height is now a share of the wizard's height (220): skeleton and grunt 0.3 (66), Guard 0.42
  (92), Brute 0.5 (110), commander 1.2 (264). Before: skeleton 80, grunt 50, Guard 105, Brute 110, commander 180, each
  a fixed number. Widths are a share of the height. Health bars follow the new heights. All five shares are in `Wiz.Tune`.
- **Blink.** Space dashes toward the mouse: to the aim point, at most 700 units, short of walls and water, in 0.5
  seconds, eased in and out. Enemies add nothing to Overwhelm during it. A 3 second cooldown starts when it ends. The
  HUD shows "Blink (Space)" or the seconds left. No effect yet; he is visible during the dash.
- **Keys.** One table (`AWizardPawn::GetKeyBindings`) holds every key: WASD, left mouse Bolt, right mouse Spear,
  Q Summon, Space Blink, E shrine, P pause, R restart, 1 2 3 cameras, H debug stats. Removed: E for the spear, Space
  for Summon, Escape (pause and closing the shrine), F for the shrine (kept free for commanding the army). The pause
  menu lists the controls.
- **Debug in the shipped build.** H and 1, 2, 3 are no longer compiled out. The stats start hidden in Shipping.
- **Robe.** A re-skin plan is in the design document under "Look and sound". Nothing was changed on the model.

## Tests

- 9 new unit tests (2 sizes, 5 Blink, 2 keys). All 9 failed against stand-ins that kept the old behavior, then passed.
  97 pass.
- New scenario `day_five_blink`. `day_two_shrine` changed because the direction changed: it pressed F and Escape, and
  now presses E for both. 9 scenarios pass.
- Screenshot looked at: `Images/2026-11-13-03-unit-sizes.png` (skeletons beside the wizard, the commander at the top).

## Build

- Shipping web package: `BUILD SUCCESSFUL`.
- Loaded in the desktop app's browser pane from a local server (deferred renderer), in the Outskirts: the HUD shows
  "Blink (Space)"; Space dashed the wizard toward the mouse, he stopped at a wall, and the HUD counted the cooldown
  down from 3 seconds; H showed the debug rings. The stats panel sits at the right edge, outside what the pane's
  screenshot covers, so the panel itself was seen only in the desktop screenshot. The camera keys were not pressed in
  the browser (the desktop scenario `day_one_input` presses them).
- After the package, one debug line was shortened ("Heights: ...") because it ran off the screen at 1280x720. That
  one-line change is in the source and not in this package.
- No zip.

## Not done, not checked

- Not play-tested by a person.
- Blink has no effect, sound or animation.
- The blink is not swept against the wizard's own collision: it trusts the simulation's walls, which are the same boxes.
- Archers, assassins, command mode: next build.
- No itch.io zip was made (the director said not to unless told). Nothing was uploaded.

## Tools used

Claude Code (Claude Fable 5.2) with the unreal-toolkit plugin; Unreal Engine 5.9 Preview.
