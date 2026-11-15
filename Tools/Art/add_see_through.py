"""Headless Unreal: makes the houses and the trees placed in the level cut away around the cursor, as the plain
boxes that stood for them used to, so the player can see and aim behind them. Walls, the bridge, lamp posts and
anything added later (the castle wall, the gate) are not touched, so they stay solid to the eye.

- /Game/Level/MPC_SeeThrough: a material parameter collection with one value, "Cursor" (R, G: the cursor on screen,
  0 to 1 across and down; B: the screen's width over its height; A: the circle's radius as a share of the screen's
  height). The game sets it every frame (ACrowdRenderer::SetSeeThrough).
- Each material in MATERIALS gets the same hole as M_SeeThrough, read from the collection, multiplied into whatever
  opacity mask it had. A material that was opaque becomes masked. Its own nodes are not moved or changed.

A material is done once (it is marked). Run with the editor closed: ue.py py --render Tools/Art/add_see_through.py"""
from ue_bp import *  # out, fail, run_step
import unreal

FOLDER = "/Game/Level"
COLLECTION = "MPC_SeeThrough"
MATERIALS = [
    "/Game/Buildings/M_Barn",
    "/Game/Buildings/M_Burned_Cottage",
    "/Game/Buildings/M_Peasant_Cottage",
    "/Game/Buildings/M_Twostory_House",
    "/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Master_Materials/M_Environment_Basic",
    "/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Master_Materials/M_Foliage_Leaves_Master_STARTER",
]
MARK = "SeeThroughHole"
# Inside the circle this much of the surface is left, and its edge is this soft (shares of the screen's height).
LEAST = 0.12
SOFT = 0.04

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
state = {}


def node(mat, cls, x, y, **props):
    made = MEL.create_material_expression(mat, cls, x, y)
    for key, value in props.items():
        made.set_editor_property(key, value)
    return made


def link(a, a_out, b, b_in):
    if not MEL.connect_material_expressions(a, a_out, b, b_in):
        fail("could not connect %s.%s to %s.%s" % (a.get_name(), a_out, b.get_name(), b_in))


def collection():
    path = FOLDER + "/" + COLLECTION
    made = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
    if made:
        out("already there:", path)
    else:
        made = tools.create_asset(COLLECTION, FOLDER, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
        value = unreal.CollectionVectorParameter()
        value.set_editor_property("parameter_name", "Cursor")
        # Off the screen with no radius: no hole until the game says where the cursor is.
        value.set_editor_property("default_value", unreal.LinearColor(-10.0, -10.0, 1.78, 0.0))
        made.set_editor_property("vector_parameters", [value])
        EAL.save_loaded_asset(made, False)
        out("made:", path)
    state["collection"] = made


def hole(mat):
    """The nodes that make the hole, placed clear of the material's own. Returns the node whose output is 1 (kept) or 0 (cut)."""
    x, y = 3000, 3000
    cursor = node(mat, unreal.MaterialExpressionCollectionParameter, x, y + 200, collection=state["collection"], parameter_name="Cursor")
    place = node(mat, unreal.MaterialExpressionComponentMask, x + 250, y + 200, r=True, g=True, b=False, a=False)
    link(cursor, "", place, "")
    aspect = node(mat, unreal.MaterialExpressionComponentMask, x + 250, y + 360, r=False, g=False, b=True, a=False)
    link(cursor, "", aspect, "")
    radius = node(mat, unreal.MaterialExpressionComponentMask, x + 250, y + 520, r=False, g=False, b=False, a=True)
    link(cursor, "", radius, "")
    screen = node(mat, unreal.MaterialExpressionScreenPosition, x, y)
    offset = node(mat, unreal.MaterialExpressionSubtract, x + 500, y + 100)
    link(screen, "ViewportUV", offset, "A")
    link(place, "", offset, "B")
    # Across counts for more than down on a wide screen: both are measured in screen heights, so the hole is round.
    stretch = node(mat, unreal.MaterialExpressionAppendVector, x + 500, y + 360)
    link(aspect, "", stretch, "A")
    link(node(mat, unreal.MaterialExpressionConstant, x + 250, y + 680, r=1.0), "", stretch, "B")
    scaled = node(mat, unreal.MaterialExpressionMultiply, x + 700, y + 200)
    link(offset, "", scaled, "A")
    link(stretch, "", scaled, "B")
    length = node(mat, unreal.MaterialExpressionLength, x + 900, y + 200)
    link(scaled, "", length, "")
    past = node(mat, unreal.MaterialExpressionSubtract, x + 1100, y + 240)
    link(length, "", past, "A")
    link(radius, "", past, "B")
    edge = node(mat, unreal.MaterialExpressionDivide, x + 1300, y + 280)
    link(past, "", edge, "A")
    link(node(mat, unreal.MaterialExpressionConstant, x + 1100, y + 440, r=SOFT), "", edge, "B")
    clamp = node(mat, unreal.MaterialExpressionSaturate, x + 1500, y + 280)
    link(edge, "", clamp, "")
    alpha = node(mat, unreal.MaterialExpressionLinearInterpolate, x + 1700, y + 360)
    link(node(mat, unreal.MaterialExpressionConstant, x + 1500, y + 480, r=LEAST), "", alpha, "A")
    link(node(mat, unreal.MaterialExpressionConstant, x + 1500, y + 580, r=1.0), "", alpha, "B")
    link(clamp, "", alpha, "Alpha")
    # A fixed speckle on screen: a pixel is kept when the opacity is above that pixel's own threshold.
    dither = node(mat, unreal.MaterialExpressionCustom, x + 1900, y + 360)
    dither.set_editor_property("code", "float t = frac(52.9829189 * frac(dot(floor(Pix), float2(0.06711056, 0.00583715)))); return Alpha > t ? 1.0 : 0.0;")
    dither.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    first = unreal.CustomInput()
    first.set_editor_property("input_name", "Alpha")
    second = unreal.CustomInput()
    second.set_editor_property("input_name", "Pix")
    dither.set_editor_property("inputs", [first, second])
    link(alpha, "", dither, "Alpha")
    link(screen, "PixelPosition", dither, "Pix")
    return dither


def materials():
    if not state.get("collection"):
        fail("no collection")
        return
    for path in MATERIALS:
        mat = unreal.load_asset(path)
        if not mat:
            fail("missing " + path)
            continue
        if EAL.get_metadata_tag(mat, MARK):
            out("already done:", path)
            continue
        kept = hole(mat)
        before = MEL.get_material_property_input_node(mat, unreal.MaterialProperty.MP_OPACITY_MASK)
        if before:
            # What it already cut (leaves) stays cut: both must keep a pixel.
            output = MEL.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_OPACITY_MASK)
            both = node(mat, unreal.MaterialExpressionMultiply, 5200, 3300)
            link(before, output, both, "A")
            link(kept, "", both, "B")
            kept = both
        if not MEL.connect_material_property(kept, "", unreal.MaterialProperty.MP_OPACITY_MASK):
            fail("could not connect the opacity mask of " + path)
            continue
        was = mat.get_editor_property("blend_mode")
        if was == unreal.BlendMode.BLEND_OPAQUE:
            mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        try:
            MEL.recompile_material(mat)
        except RuntimeError as error:
            out("not compiled here:", str(error)[:90])
        EAL.set_metadata_tag(mat, MARK, "1")
        out("hole added:", path, "had a mask:", bool(before), "was", str(was).split(".")[-1].split(":")[0], "saved:", EAL.save_loaded_asset(mat, False))


run_step("collection", collection)
run_step("materials", materials)
