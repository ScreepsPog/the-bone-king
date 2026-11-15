"""Headless Unreal: imports the Paragon animations picked out for the director (the folder
_Picks_For_The_Bone_King inside the Paragon folder) as animation sequences on the mannequin's skeleton, so they can
be opened and watched in the editor.

They go to /Game/Paragon_Picks/<the pick's folder>/A_<Hero>_<name>. That folder is NOT in the list of what a web
build must include: an animation costs nothing in the game until something uses it.
One that is already there is left alone. Run with the editor closed: ue.py py Tools/Art/import_paragon_picks.py"""
from ue_bp import *  # out, fail, run_step
import math
import os
import re
import unreal

SOURCE = os.environ.get("PARAGON_PICKS", "C:/Users/<user>/OneDrive/Desktop/ParagonAnimationsRetargetedToManny/_Picks_For_The_Bone_King")
DEST = "/Game/Paragon_Picks"
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"

EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def facing(sequence):
    """How far the pose is turned from the mannequin's own facing, in degrees, from the line between the feet at the start."""
    try:
        options = unreal.AnimPoseEvaluationOptions()
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_frame(sequence, 0, options)
        left = unreal.AnimPoseExtensions.get_bone_pose(pose, "thigh_l", unreal.AnimPoseSpaces.WORLD).translation
        right = unreal.AnimPoseExtensions.get_bone_pose(pose, "thigh_r", unreal.AnimPoseSpaces.WORLD).translation
        return round(math.degrees(math.atan2(left.y - right.y, left.x - right.x)))
    except Exception as error:
        return str(error)[:60]


def make():
    skeleton = unreal.load_asset(SKELETON)
    if not skeleton:
        fail("missing " + SKELETON)
        return
    if not os.path.isdir(SOURCE):
        fail("missing " + SOURCE)
        return
    made = 0
    there = 0
    turned = {}
    for folder in sorted(os.listdir(SOURCE)):
        path = SOURCE + "/" + folder
        if not os.path.isdir(path) or folder.startswith("_"):
            continue
        for file in sorted(os.listdir(path)):
            if not file.lower().endswith(".fbx"):
                continue
            name = "A_" + re.sub(r"[^A-Za-z0-9_]", "_", file[:-4].replace("__", "_"))
            dest = DEST + "/" + folder
            asset = dest + "/" + name
            if EAL.does_asset_exist(asset):
                there += 1
            else:
                ui = unreal.FbxImportUI()
                ui.set_editor_property("import_mesh", False)
                ui.set_editor_property("import_as_skeletal", True)
                ui.set_editor_property("import_animations", True)
                ui.set_editor_property("import_materials", False)
                ui.set_editor_property("import_textures", False)
                ui.set_editor_property("skeleton", skeleton)
                ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
                ui.set_editor_property("automated_import_should_detect_type", False)
                task = unreal.AssetImportTask()
                task.filename = path + "/" + file
                task.destination_path = dest
                task.destination_name = name
                task.automated = True
                task.replace_existing = True
                task.save = True
                task.options = ui
                tools.import_asset_tasks([task])
                if not EAL.does_asset_exist(asset):
                    fail("did not import " + folder + "/" + file)
                    continue
                made += 1
            sequence = unreal.load_asset(asset)
            angle = facing(sequence)
            turned.setdefault(angle, 0)
            turned[angle] += 1
            out(folder + "/" + name, "%.2f s" % sequence.get_play_length(), "feet line", angle)
    out("imported", made, "already there", there, "feet-line angles (0 or 180 is square to the mannequin):", turned)


run_step("paragon", make)
