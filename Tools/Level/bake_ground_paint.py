"""Bakes where the landscape is painted dirt and where grass into the game's settings (Config/DefaultGame.ini,
section [WizardGroundPaint]), so a packaged game can pick the sound of a body falling: the paint itself cannot be
read there. Run again after repainting the landscape.

Run (editor closed): ue.py py Tools/Level/bake_ground_paint.py
"""
import re
import unreal

MAP = "/Game/Maps/Outskirts"
# Cells each way over the whole landscape.
SIZE = 128
# The paint layers that sound of dirt; every other layer sounds of grass.
DIRT_LAYERS = ["Ground", "Dirt", "DirtySoil", "Stone"]
INI = unreal.Paths.project_config_dir() + "DefaultGame.ini"

unreal.EditorLoadingAndSavingUtils.load_map(MAP)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
land = next((a for a in actors if a.get_class().get_name() == "Landscape"), None)
baked = unreal.CrowdRenderer.bake_ground_paint(land, SIZE, DIRT_LAYERS) if land else ""
if "|" not in baked:
    print("GROUND failed: no landscape, or no paint read")
else:
    bounds, cells = baked.split("|")
    min_x, min_y, max_x, max_y = bounds.split(",")
    block = "[WizardGroundPaint]\nWidth=%d\nHeight=%d\nMinX=%s\nMinY=%s\nMaxX=%s\nMaxY=%s\nCells=%s\n" % (SIZE, SIZE, min_x, min_y, max_x, max_y, cells)
    text = open(INI, encoding="utf-8").read().replace("\r\n", "\n")
    text = re.sub(r"\n*\[WizardGroundPaint\]\n(?:[^\[\n][^\n]*\n?)*", "\n", text).rstrip("\n") + "\n\n" + block
    open(INI, "w", encoding="utf-8", newline="\n").write(text)
    print("GROUND baked", SIZE, "x", SIZE, "from", bounds, "dirt cells", cells.count("1"), "of", len(cells))
    # A picture for the eye: every fourth cell.
    for row in range(0, SIZE, 4):
        print("GROUND row", "".join("#" if cells[row * SIZE + col] == "1" else "." for col in range(0, SIZE, 2)))
