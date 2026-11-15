"""Headless Unreal: caps the size of the big textures, to make the game's download smaller.

The camera looks down from far above, so a 2048 texture on a house or a tree shows no more than a 1024 one, and it is
four times the bytes. This sets "Maximum Texture Size" on every texture in the folders below that is larger than the
cap. The source picture inside the asset is kept whole: set the value back to 0 in the texture editor to undo it.

  /Game/Buildings, /Game/StyleHex_Studio, /Game/Kingdom, /Game/Props, /Game/Megascans   1024
  /Game/Characters/Mannequins (the engine's mannequin: it is loaded for its animations, never seen)   256

Run with the editor closed: ue.py py Tools/Art/cap_texture_sizes.py
"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import unreal

CAPS = (
    ("/Game/Buildings", 1024),
    ("/Game/StyleHex_Studio", 1024),
    ("/Game/Kingdom", 1024),
    ("/Game/Props", 1024),
    ("/Game/Megascans", 1024),
    ("/Game/Characters/Mannequins", 256),
)
changed = []


def cap():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    total = 0
    for folder, limit in CAPS:
        for data in registry.get_assets_by_path(folder, recursive=True):
            if str(data.asset_class_path.asset_name) != "Texture2D":
                continue
            texture = data.get_asset()
            width = texture.blueprint_get_size_x()
            height = texture.blueprint_get_size_y()
            now = texture.get_editor_property("max_texture_size")
            largest = max(width, height)
            if largest <= limit or (0 < now <= limit):
                continue
            texture.set_editor_property("max_texture_size", limit)
            changed.append(texture)
            total += 1
            out("capped %s: %dx%d to %d" % (data.package_name, width, height, limit))
    out("textures capped:", total)


run_step("cap", cap)
if changed:
    save_if_clean(*changed)
