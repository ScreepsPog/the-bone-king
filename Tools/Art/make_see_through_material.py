"""Headless Unreal: makes /Game/Level/M_SeeThrough, the material buildings and walls are drawn with.

It is the basic shape look (one color) with a hole: inside a circle around the cursor on screen the surface is mostly
cut away (dithered), so the player can see and aim at what stands behind a building.

Parameters, set from C++ every frame (ACrowdRenderer):
  Color   the piece's color
  Cursor  R, G: the cursor on screen, 0 to 1 across and down. B: the screen's width over its height.
          A: the circle's radius as a share of the screen's height (0 switches it off).

Makes only what is missing. Delete Content/Level/M_SeeThrough.uasset to make it again.
Run with the editor closed: ue.py py Tools/Art/make_see_through_material.py
"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import unreal

DEST = "/Game/Level"
NAME = "M_SeeThrough"
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
made = []


def node(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def link(a, a_out, b, b_in):
    if not MEL.connect_material_expressions(a, a_out, b, b_in):
        fail("could not connect %s.%s to %s.%s" % (a.get_name(), a_out, b.get_name(), b_in))


def material():
    path = DEST + "/" + NAME
    if EAL.does_asset_exist(path):
        out("already there:", path)
        made.append(unreal.load_asset(path))
        return
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    # Every piece of the level is an instanced mesh: without this flag a packaged game draws the gray default instead.
    mat.set_editor_property("used_with_instanced_static_meshes", True)

    color = node(mat, unreal.MaterialExpressionVectorParameter, -500, -200)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.3, 0.3, 0.3, 1.0))
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = node(mat, unreal.MaterialExpressionConstant, -500, 0)
    rough.set_editor_property("r", 0.8)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    cursor = node(mat, unreal.MaterialExpressionVectorParameter, -1500, 300)
    cursor.set_editor_property("parameter_name", "Cursor")
    cursor.set_editor_property("default_value", unreal.LinearColor(-10.0, -10.0, 1.78, 0.0))

    screen = node(mat, unreal.MaterialExpressionScreenPosition, -1500, 100)
    cursor_uv = node(mat, unreal.MaterialExpressionAppendVector, -1250, 300)
    link(cursor, "R", cursor_uv, "A")
    link(cursor, "G", cursor_uv, "B")
    offset = node(mat, unreal.MaterialExpressionSubtract, -1050, 200)
    link(screen, "ViewportUV", offset, "A")
    link(cursor_uv, "", offset, "B")

    # Across counts for more than down on a wide screen: both are measured in screen heights, so the hole is round.
    one = node(mat, unreal.MaterialExpressionConstant, -1250, 480)
    one.set_editor_property("r", 1.0)
    stretch = node(mat, unreal.MaterialExpressionAppendVector, -1050, 420)
    link(cursor, "B", stretch, "A")
    link(one, "", stretch, "B")
    scaled = node(mat, unreal.MaterialExpressionMultiply, -850, 260)
    link(offset, "", scaled, "A")
    link(stretch, "", scaled, "B")
    length = node(mat, unreal.MaterialExpressionLength, -680, 260)
    link(scaled, "", length, "")

    # 0 inside the circle, rising to 1 over a soft edge.
    past = node(mat, unreal.MaterialExpressionSubtract, -520, 300)
    link(length, "", past, "A")
    link(cursor, "A", past, "B")
    soft = node(mat, unreal.MaterialExpressionScalarParameter, -700, 480)
    soft.set_editor_property("parameter_name", "Soft")
    soft.set_editor_property("default_value", 0.04)
    edge = node(mat, unreal.MaterialExpressionDivide, -360, 340)
    link(past, "", edge, "A")
    link(soft, "", edge, "B")
    clamp = node(mat, unreal.MaterialExpressionSaturate, -220, 340)
    link(edge, "", clamp, "")

    # Inside the circle a little of the building is left, so it is seen to be there.
    least = node(mat, unreal.MaterialExpressionScalarParameter, -400, 520)
    least.set_editor_property("parameter_name", "Least")
    least.set_editor_property("default_value", 0.12)
    full = node(mat, unreal.MaterialExpressionConstant, -400, 640)
    full.set_editor_property("r", 1.0)
    alpha = node(mat, unreal.MaterialExpressionLinearInterpolate, -80, 420)
    link(least, "", alpha, "A")
    link(full, "", alpha, "B")
    link(clamp, "", alpha, "Alpha")
    # A fixed speckle on screen: each pixel is kept when the opacity is above that pixel's own threshold, so 12%
    # opacity leaves about one pixel in eight. (The engine's dither is a material function, not reachable from here.)
    dither = node(mat, unreal.MaterialExpressionCustom, 80, 420)
    dither.set_editor_property("code", "float t = frac(52.9829189 * frac(dot(floor(Pix), float2(0.06711056, 0.00583715)))); return Alpha > t ? 1.0 : 0.0;")
    dither.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    first = unreal.CustomInput()
    first.set_editor_property("input_name", "Alpha")
    second = unreal.CustomInput()
    second.set_editor_property("input_name", "Pix")
    dither.set_editor_property("inputs", [first, second])
    link(alpha, "", dither, "Alpha")
    link(screen, "PixelPosition", dither, "Pix")
    if not MEL.connect_material_property(dither, "", unreal.MaterialProperty.MP_OPACITY_MASK):
        fail("could not connect the opacity mask")
    made.append(mat)
    mask = MEL.get_material_property_input_node(mat, unreal.MaterialProperty.MP_OPACITY_MASK)
    out("made", path, "opacity mask from", mask.get_name() if mask else None)
    if not mask:
        fail("the opacity mask is not connected")


run_step("material", material)
save_if_clean(*made)
