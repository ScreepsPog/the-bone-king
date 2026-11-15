"""Darkens the leaves of the level's trees to sit in the night: the two leaf material instances of the StyleHex
forest pack get darker, slightly grayer colors. The colors here are the pack's own; the result is always made from
them, so the script can be run again with another DARKEN and nothing drifts.

Run (editor closed): ue.py py Tools/Art/darken_leaves.py
"""
import unreal

# The share of the pack's brightness the leaves keep (1 = as the pack made them).
DARKEN = 0.38
# How far each color is pulled toward gray (0 = as vivid as the pack, 1 = gray).
GRAY = 0.3
# What shines through a leaf from the light behind it keeps less still: a bright rim reads as daylight.
SUBSURFACE = 0.25

BASE = "/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample/Materials/Mat_Trees/"
ORIGINAL = {
    "MI_Tree_Leaves_Birch_A": {
        "Color Base": (0.0689, 0.2384, 0.0),
        "Color Top": (0.1686, 0.4271, 0.0667),
        "Subsurface Color": (0.0197, 0.0677, 0.0391),
    },
    "MI_Tree_Leaves_Broadleaf": {
        "Color Base": (0.0131, 0.125, 0.0508),
        "Color Top": (0.1789, 0.4531, 0.0708),
        "Subsurface Color": (0.0171, 0.1042, 0.0521),
    },
}


def darker(color, keep):
    gray = 0.2126 * color[0] + 0.7152 * color[1] + 0.0722 * color[2]
    return unreal.LinearColor(*[(c + (gray - c) * GRAY) * keep for c in color], 1.0)


MEL = unreal.MaterialEditingLibrary
for name, colors in ORIGINAL.items():
    instance = unreal.load_asset(BASE + name)
    for parameter, color in colors.items():
        value = darker(color, SUBSURFACE if parameter == "Subsurface Color" else DARKEN)
        MEL.set_material_instance_vector_parameter_value(instance, parameter, value)
        print("LEAVES", name, parameter, round(value.r, 4), round(value.g, 4), round(value.b, 4))
    MEL.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_asset(BASE + name, only_if_is_dirty=False)
print("LEAVES done")
