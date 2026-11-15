"""Headless Unreal: imports a building made by Tools/Art/prepare_building_glb.py, or a weapon made by
Tools/Art/prepare_weapon_obj.py, as a static mesh with its material. Makes only what is missing.
Run: ART_BUILDING=<name> ue.py py Tools/Art/import_building.py   (ART_BUILDING=All does every one)
     ART_SET=Weapons ue.py py Tools/Art/import_building.py        (the weapons, into /Game/Weapons)
     ART_SET=Props ue.py py Tools/Art/import_building.py          (props made by prepare_prop_obj.py, into /Game/Props)"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import os
import unreal

SET = os.environ.get("ART_SET", "Buildings")
SOURCE = "C:/Dev/WebTesting/SourceArt/" + SET
DEST = "/Game/" + SET
# The files are 2048. Color and normal keep all of it (at 1024 a house eleven meters wide read as blurred); the
# roughness and metal map is smooth, so half is enough. Smaller mips are sharpened so the walls stay crisp from the
# game's camera.
# A weapon is a meter long in a hand seen from above: 512 is as much as shows.
# A prop is a few meters tall and stands twenty times in a level: 512 as well.
MAX_SIZES = {"BaseColor": 2048, "Normal": 2048, "RM": 1024} if SET in ("Buildings", "Kingdom") else {"BaseColor": 512, "Normal": 512, "RM": 512}
BUILDINGS = {"Weapons": ["Iron_Longsword", "Steel_Sword", "Commander_Sword", "Wood_Bow", "Skeleton_Bow", "Steel_Hammer", "Archer_Arrow", "Marksman_Arrow", "Wizard_Spear", "Commander_Spear", "Dagger"],
             "Props": ["Lamp_Post", "Bone_Pile", "Bridge"],
             # The kingdom's wall, made by prepare_prop_obj.py into SourceArt/Kingdom: as large as houses.
             "Kingdom": ["Castle_Wall", "Castle_Gate", "Castle_Gate_Door", "Castle_Tower"]}.get(
    SET, ["Peasant_Cottage", "Burned_Cottage", "Barn", "Twostory_House", "Shrine"])
# Only these of the set, when ART_NAMES is given (comma separated): ART_NAMES=Wood_Bow,Skeleton_Bow
if os.environ.get("ART_NAMES"):
    BUILDINGS = [name for name in BUILDINGS if name in os.environ["ART_NAMES"].split(",")]

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


def find_file(stem):
    for ending in (".jpg", ".png"):
        if os.path.exists(SOURCE + "/" + stem + ending):
            return SOURCE + "/" + stem + ending
    return None


def textures(name):
    for kind in ("BaseColor", "Normal", "RM"):
        asset = "T_" + name + "_" + kind
        path = DEST + "/" + asset
        if EAL.does_asset_exist(path):
            out("already there:", path)
        else:
            file = find_file(name + "_" + kind)
            if not file:
                fail("no file for " + asset)
                continue
            out("imported:", import_file(file, asset))
        tex = unreal.load_asset(path)
        if not tex:
            fail("texture missing: " + path)
            continue
        if kind == "Normal":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
        elif kind == "RM":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        tex.set_editor_property("max_texture_size", MAX_SIZES[kind])
        if kind == "BaseColor":
            tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SHARPEN4)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP if kind == "Normal" else unreal.TextureGroup.TEXTUREGROUP_WORLD)
        made.append(tex)
        out(asset, tex.blueprint_get_size_x(), "x", tex.blueprint_get_size_y(), "source, capped at", MAX_SIZES[kind])


def material(name):
    path = DEST + "/M_" + name
    mat = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
    if mat:
        out("already there:", path)
    else:
        mat = tools.create_asset("M_" + name, DEST, unreal.Material, unreal.MaterialFactoryNew())
        rows = [
            ("BaseColor", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, [("RGB", unreal.MaterialProperty.MP_BASE_COLOR)]),
            ("Normal", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, [("RGB", unreal.MaterialProperty.MP_NORMAL)]),
            # Roughness is in green and metallic in blue, as glTF packs them.
            ("RM", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, [("G", unreal.MaterialProperty.MP_ROUGHNESS), ("B", unreal.MaterialProperty.MP_METALLIC)]),
        ]
        for i, (kind, sampler, pins) in enumerate(rows):
            node = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -420, i * 260)
            node.set_editor_property("texture", unreal.load_asset(DEST + "/T_" + name + "_" + kind))
            node.set_editor_property("sampler_type", sampler)
            for pin, prop in pins:
                if not MEL.connect_material_property(node, pin, prop):
                    fail("could not connect " + kind + " " + pin)
    made.append(mat)
    return mat


def mesh(name, mat):
    path = DEST + "/SM_" + name
    if EAL.does_asset_exist(path):
        out("already there:", path)
    else:
        ui = unreal.FbxImportUI()
        ui.set_editor_property("import_mesh", True)
        ui.set_editor_property("import_as_skeletal", False)
        ui.set_editor_property("import_materials", False)
        ui.set_editor_property("import_textures", False)
        ui.set_editor_property("import_animations", False)
        ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        data = ui.static_mesh_import_data
        data.set_editor_property("combine_meshes", True)
        data.set_editor_property("auto_generate_collision", False)
        data.set_editor_property("generate_lightmap_u_vs", False)
        data.set_editor_property("import_uniform_scale", 1.0)
        out("imported:", import_file(SOURCE + "/" + name + ".fbx", "SM_" + name, ui))
    sm = unreal.load_asset(path)
    if not sm:
        fail("mesh missing: " + path)
        return
    for index in range(len(sm.static_materials)):
        if sm.get_material(index) != mat:
            sm.set_material(index, mat)
    box = sm.get_bounding_box()
    out("SM_" + name, "size", round(box.max.x - box.min.x), round(box.max.y - box.min.y), round(box.max.z - box.min.z), "from z", round(box.min.z), "to", round(box.max.z),
        "materials", [str(sm.get_material(i).get_name()) for i in range(len(sm.static_materials))])
    made.append(sm)


def one(name):
    textures(name)
    mat = material(name)
    mesh(name, mat)


wanted = os.environ.get("ART_BUILDING", "All")
for building in BUILDINGS:
    if wanted in ("All", building):
        run_step(building, lambda b=building: one(b))
save_if_clean(*made)
