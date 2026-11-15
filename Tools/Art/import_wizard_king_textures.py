"""Headless Unreal: imports a character model's textures (capped in size) and makes its material. Makes only what is missing.
The mesh itself comes from Tools/Art/import_mannequin_character.py. Run: ue.py py Tools/Art/import_wizard_king_textures.py"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import os
import unreal

# Which model: set the environment variable ART_MODEL (default WizardKing) before running.
#   ART_MODEL=Skeleton ue.py py Tools/Art/import_wizard_king_textures.py
MODELS = {
    # source folder, content folder, mesh, material, largest texture size, file name stem, file ending, texture name stem
    "WizardKing": ("E:/UE5/AI Game Jam Assets/Models/Wizard King/Wizard_KingV2", "/Game/Wizard", "SKM_Wizard_King", "M_Wizard_King", 1024,
                   "skeleton_king_3d_model_", ".JPEG", "T_Wizard_King_"),
    "Skeleton": ("E:/UE5/AI Game Jam Assets/Models/Friendly_AI_Army/01_Base/tripo_convert_6c689391-639b-4aca-bea6-ff66472e7c77.fbm", "/Game/Army",
                 "SKM_Skeleton", "M_Skeleton", 512, "skeleton_3d_model_", ".jpg", "T_Skeleton_"),
    "Guard": ("E:/UE5/AI Game Jam Assets/Models/Friendly_AI_Army/02_Guard/tripo_convert_9d4a94c4-49a1-4b4e-88bc-60e6e4b7a1bb.fbm", "/Game/Army",
              "SKM_Guard", "M_Guard", 512, "fantasy_skeleton_knight_3d_model_", ".jpg", "T_Guard_"),
    "Grunt": ("E:/UE5/AI Game Jam Assets/Models/Enemy_AI/01_Base/tripo_convert_6d0fe61c-9bd8-44f1-accb-f2157619a1a1.fbm", "/Game/Enemy",
              "SKM_Grunt", "M_Grunt", 512, "medieval_knight_3d_model_", ".jpg", "T_Grunt_"),
    "Assassin": ("E:/UE5/AI Game Jam Assets/Models/Enemy_AI/04_Assassin/tripo_convert_bba3d2a0-ab9c-4f38-b25c-455ccce5d029.fbm", "/Game/Enemy",
                 "SKM_Assassin", "M_Assassin", 512, "sun_emblem_armor_3d_model_", ".jpg", "T_Assassin_"),
    "Archer": ("E:/UE5/AI Game Jam Assets/Models/Enemy_AI/03_Archer/tripo_convert_1f2ba2bd-d179-48f0-83d9-d9b33392f79b.fbm", "/Game/Enemy",
               "SKM_Archer", "M_Archer", 512, "fantasy_archer_3d_model_", ".jpg", "T_Archer_"),
    "Brute": ("E:/UE5/AI Game Jam Assets/Models/Enemy_AI/02_Brute/tripo_convert_016e7dd4-3ea1-40f4-9b03-38ff49adc57f.fbm", "/Game/Enemy",
              "SKM_Brute", "M_Brute", 512, "medieval_knight_3d_model_", ".jpg", "T_Brute_"),
    "Commander": ("E:/UE5/AI Game Jam Assets/Models/Enemy_AI/05_Commander/tripo_convert_2adabb70-318b-4abb-95f2-8664d8c4ac25.fbm", "/Game/Enemy",
                  "SKM_Commander", "M_Commander", 512, "medieval_knight_3d_model_", ".jpg", "T_Commander_"),
    "Marksman": ("E:/UE5/AI Game Jam Assets/Models/Friendly_AI_Army/03_Marksman/tripo_convert_3f13f707-385e-4742-b1e6-b734da114680.fbm", "/Game/Army",
                 "SKM_Marksman", "M_Marksman", 512, "skeleton_archer_3d_model_", ".jpg", "T_Marksman_"),
}
SRC, DEST, MESH_NAME, MATERIAL_NAME, MAX_SIZE, FILE_STEM, FILE_END, TEX_STEM = MODELS[os.environ.get("ART_MODEL", "WizardKing")]
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
MESH = DEST + "/" + MESH_NAME
MATERIAL = DEST + "/" + MATERIAL_NAME
TEXTURES = {
    TEX_STEM + "BaseColor": (FILE_STEM + "basecolor" + FILE_END, "color"),
    TEX_STEM + "Normal": (FILE_STEM + "normal" + FILE_END, "normal"),
    TEX_STEM + "Roughness": (FILE_STEM + "roughness" + FILE_END, "mask"),
    TEX_STEM + "Metallic": (FILE_STEM + "metallic" + FILE_END, "mask"),
}
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
made = []


def import_file(filename, name, options=None):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = DEST
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    if options:
        task.options = options
    tools.import_asset_tasks([task])
    return [str(p) for p in task.imported_object_paths]


def textures():
    for name, (file, kind) in TEXTURES.items():
        path = DEST + "/" + name
        if EAL.does_asset_exist(path):
            out("already there:", path)
        else:
            out("imported:", import_file(SRC + "/" + file, name))
        tex = unreal.load_asset(path)
        if not tex:
            fail("texture missing: " + path)
            continue
        if kind == "normal":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
        elif kind == "mask":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
            tex.set_editor_property("srgb", False)
        tex.set_editor_property("max_texture_size", MAX_SIZE)
        made.append(tex)
        out(name, tex.blueprint_get_size_x(), "x", tex.blueprint_get_size_y(), "source, capped at", MAX_SIZE)


def material():
    mat = unreal.load_asset(MATERIAL) if EAL.does_asset_exist(MATERIAL) else None
    if mat:
        out("already there:", MATERIAL)
    else:
        mat = tools.create_asset(MATERIAL_NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
        rows = [
            (TEX_STEM + "BaseColor", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, "RGB", unreal.MaterialProperty.MP_BASE_COLOR),
            (TEX_STEM + "Normal", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, "RGB", unreal.MaterialProperty.MP_NORMAL),
            (TEX_STEM + "Roughness", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE, "R", unreal.MaterialProperty.MP_ROUGHNESS),
            (TEX_STEM + "Metallic", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE, "R", unreal.MaterialProperty.MP_METALLIC),
        ]
        for i, (tex_name, sampler, pin, prop) in enumerate(rows):
            node = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -420, i * 260)
            node.set_editor_property("texture", unreal.load_asset(DEST + "/" + tex_name))
            node.set_editor_property("sampler_type", sampler)
            if not MEL.connect_material_property(node, pin, prop):
                fail("could not connect " + tex_name)
    # Without this flag the engine draws a skinned mesh with the gray default material.
    if not mat.get_editor_property("used_with_skeletal_mesh"):
        mat.set_editor_property("used_with_skeletal_mesh", True)
        out("set: used with skeletal mesh")
    made.append(mat)
    sk = unreal.load_asset(MESH)
    if sk:
        slots = list(sk.materials)
        changed = False
        for slot in slots:
            if slot.material_interface != mat:
                slot.material_interface = mat
                changed = True
        if changed:
            sk.set_editor_property("materials", slots)
        out("mesh materials:", [(str(m.material_slot_name), str(m.material_interface.get_name()) if m.material_interface else None) for m in sk.materials])


run_step("textures", textures)
run_step("material", material)
save_if_clean(*made)
