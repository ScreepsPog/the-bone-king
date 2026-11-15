"""Headless Unreal: imports the HUD's icons into /Game/UI as textures for the canvas HUD (no mips, never streamed,
shown at 44 to 92 pixels at 1080p, so 256 is as much as a 4K screen shows), and the start menu's picture, which fills
the screen (2048 wide).

Run with the editor closed: ue.py py Tools/Art/import_ui_icons.py"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import os
import unreal

SOURCE = "E:/UE5/AI Game Jam Assets/UI"
DEST = "/Game/UI"
ICONS = {
    "T_Icon_Summon": "Icons/Skeleton_Summon.png",
    "T_Icon_Spear": "Icons/Spear_Ability.png",
    "T_Icon_Blink": "Icons/Wizard_Blink.png",
    "T_Icon_Command": "Icons/Command_Ability.png",
    "T_Icon_Bone": "Icons/Bone_Currency.png",
    # The shrine's rows.
    "T_Icon_Armor": "Icons/Bone_Armor.png",
    "T_Icon_Blades": "Icons/Bone-Blades.png",
    "T_Icon_Focus": "Icons/Dark_Focus.png",
    "T_Icon_Guard": "Icons/Skeleton_Guard.png",
    # The Marksman's row: the director gave this one in the chat; it is kept in the project's SourceArt.
    "T_Icon_Marksman": "C:/Dev/WebTesting/SourceArt/UI/Skeleton_Marksman.png",
    "T_Start_Background": "Main_Menu_Art.png",
}
MAX_SIZE = 256
BIG = {"T_Start_Background": 2048}
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
made = []


def icons():
    for name, file in ICONS.items():
        path = file if ":" in file else SOURCE + "/" + file
        if not os.path.isfile(path):
            fail("missing " + path)
            continue
        task = unreal.AssetImportTask()
        task.filename = path
        task.destination_path = DEST
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = False
        tools.import_asset_tasks([task])
        texture = EAL.load_asset(DEST + "/" + name)
        if not texture:
            fail("did not import " + name)
            continue
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("max_texture_size", BIG.get(name, MAX_SIZE))
        texture.set_editor_property("never_stream", True)
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        out(name, "source", texture.blueprint_get_size_x(), "x", texture.blueprint_get_size_y(), "capped at", BIG.get(name, MAX_SIZE))
        made.append(texture)


run_step("icons", icons)
save_if_clean(*made)
