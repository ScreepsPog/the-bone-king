"""Headless Unreal: adds the two Megascans surfaces in /Game/Megascans/Surfaces to the ground, as two more layers
that can be painted in Landscape mode: "WildGrass" and "DirtySoil".

- /Game/Landscape/M_Ground gets, after its four plain layers, two layers that show the surfaces' own color and
  normal textures (one repeat every 4 meters). What is already painted is not changed: a new layer starts empty.
- The landscape gets the two target layers, each with a layer info in /Game/Landscape, so they show in the Paint
  tab ready to use.
- The level is saved. Nothing else in it is touched.

Run with the editor closed: ue.py py --render Tools/Level/add_ground_layers.py"""
from ue_bp import *  # out, fail, run_step
import unreal

MAP = "/Game/Maps/Outskirts"
FOLDER = "/Game/Landscape"
SURFACES = "/Game/Megascans/Surfaces"
LAYERS = [
    ("WildGrass", SURFACES + "/Wild_Grass_rm5kpip0/T_Wild_Grass_rm5kpip0_1K_D", SURFACES + "/Wild_Grass_rm5kpip0/T_Wild_Grass_rm5kpip0_1K_N"),
    ("DirtySoil", SURFACES + "/Dirty_Soil_Ground_xeonbdls/T_Dirty_Soil_Ground_xeonbdls_1K_D", SURFACES + "/Dirty_Soil_Ground_xeonbdls/T_Dirty_Soil_Ground_xeonbdls_1K_N"),
]
# The layers the ground had before: their paint must come through unchanged.
OLD = ["Ground", "Grass", "Dirt", "Stone"]
# One repeat of a surface every this many meters.
REPEAT = 4.0

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
state = {}


def material():
    mat = unreal.load_asset(FOLDER + "/M_Ground")
    if not mat:
        fail("missing " + FOLDER + "/M_Ground")
        return
    state["material"] = mat
    if MEL.get_material_property_input_node(mat, unreal.MaterialProperty.MP_NORMAL):
        out("already there: the two surfaces in M_Ground")
        return
    last = MEL.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    if not last:
        fail("M_Ground has nothing on its base color")
        return
    coords = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerCoords, -1300, 2200)
    coords.set_editor_property("mapping_scale", REPEAT)
    flat = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -650, 2100)
    flat.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    last_normal = flat
    for index, (name, color_path, normal_path) in enumerate(LAYERS):
        color_texture = unreal.load_asset(color_path)
        normal_texture = unreal.load_asset(normal_path)
        if not color_texture or not normal_texture:
            fail("missing a texture of " + name)
            return
        y = 2300 + index * 700
        color = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -1000, y)
        color.set_editor_property("texture", color_texture)
        color.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        color.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        MEL.connect_material_expressions(coords, "", color, "UVs")
        weight = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerWeight, -350, y)
        weight.set_editor_property("parameter_name", name)
        weight.set_editor_property("preview_weight", 0.0)
        if not MEL.connect_material_expressions(color, "RGB", weight, "Layer") or not MEL.connect_material_expressions(last, "", weight, "Base"):
            fail("could not connect the color of " + name)
        last = weight

        normal = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -1000, y + 320)
        normal.set_editor_property("texture", normal_texture)
        normal.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        normal.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        MEL.connect_material_expressions(coords, "", normal, "UVs")
        normal_weight = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerWeight, -350, y + 320)
        normal_weight.set_editor_property("parameter_name", name)
        normal_weight.set_editor_property("preview_weight", 0.0)
        if not MEL.connect_material_expressions(normal, "RGB", normal_weight, "Layer") or not MEL.connect_material_expressions(last_normal, "", normal_weight, "Base"):
            fail("could not connect the normal of " + name)
        last_normal = normal_weight
    MEL.connect_material_property(last, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(last_normal, "", unreal.MaterialProperty.MP_NORMAL)
    try:
        MEL.recompile_material(mat)
    except RuntimeError as error:
        out("not compiled here:", str(error)[:90])
    out("M_Ground: added", [name for name, c, n in LAYERS], "expressions now", MEL.get_num_material_expressions(mat))
    state["changed"] = True


def landscape():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    editor = unreal.get_editor_subsystem(unreal.LandscapeEditorSubsystem)
    found = editor.get_landscapes()
    if not found:
        fail("no landscape in " + MAP)
        return
    land = found[0]

    def painted():
        return {name: round(sum(editor.get_weightmap_data(land, "", name)), 1) for name in OLD}

    before = painted()
    out("painted before:", before)
    names = [str(n) for n in editor.get_target_layer_names(land)]
    infos = []
    for name, color_path, normal_path in LAYERS:
        if name not in names:
            editor.add_target_layer(land, name, None)
            out("added target layer:", name)
        info = editor.get_target_layer_info(land, name)
        if not info:
            path = FOLDER + "/" + name + "_LayerInfo"
            info = unreal.load_asset(path) if EAL.does_asset_exist(path) else editor.create_target_layer_info(name, FOLDER)
            editor.set_target_layer_info(land, name, info)
            out("layer info:", name, info.get_path_name())
        infos.append(info)
    land.force_layers_full_update()
    after = painted()
    out("painted after: ", after)
    if before != after:
        fail("the paint that was there has changed")
        return
    out("layers now:", [str(n) for n in editor.get_target_layer_names(land)])
    if state.get("material"):
        try:
            MEL.recompile_material(state["material"])
        except RuntimeError as error:
            out("not compiled here:", str(error)[:90])
        out("saved material:", EAL.save_loaded_asset(state["material"], False))
    for info in infos:
        EAL.save_loaded_asset(info, False)
    out("saved level:", levels.save_current_level())


run_step("material", material)
run_step("landscape", landscape)
