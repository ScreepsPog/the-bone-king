"""Headless Unreal: imports Cinzel Bold (Tools/Art/Fonts/Cinzel, SIL Open Font License, from
https://github.com/NDISCOVER/Cinzel) as the font face /Game/UI/Fonts/FF_Cinzel_Bold, for the HUD's
titles and labels (the HUD makes its typeface from the face in code: Python cannot fill in a font asset's faces). The
face is stored inside its asset (inline), so a package needs no loose file.

Run with the editor closed: ue.py py Tools/Art/import_fonts.py"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import os
import unreal

SOURCE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Fonts", "Cinzel", "Cinzel-Bold.ttf").replace("\\", "/")
if not os.path.isfile(SOURCE):
    SOURCE = "C:/Dev/WebTesting/Tools/Art/Fonts/Cinzel/Cinzel-Bold.ttf"
DEST = "/Game/UI/Fonts"
FACE = "FF_Cinzel_Bold"
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
made = []


def make():
    if not os.path.isfile(SOURCE):
        fail("missing " + SOURCE)
        return
    task = unreal.AssetImportTask()
    task.filename = SOURCE
    task.destination_path = DEST
    task.destination_name = FACE
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    face = EAL.load_asset(DEST + "/" + FACE)
    if not face or face.get_class().get_name() != "FontFace":
        fail("did not import the face: %s" % (face.get_class().get_name() if face else None))
        return
    face.set_editor_property("loading_policy", unreal.FontLoadingPolicy.INLINE)
    made.append(face)

    out("face", FACE, "inline")


run_step("font", make)
save_if_clean(*made)
