"""Headless Unreal: makes the two effect materials the fight is drawn with, in /Game/Level.

M_FireBolt    the wizard's Bolt: a ball of fire. Drawn on the engine's Sphere with its long (Z) axis along the flight.
              White-hot in the middle, orange to red at the rim, cooler toward the tail, flickering, and it swells and
              shrinks (the pulse). Unlit and translucent, so it glows the same in any light.
M_GroundMark  a mark lying on the ground, drawn on the engine's Plane: the enemy's warnings (a slam's circle, the
              commander's spear lane, the brute's sweep) and where the wizard's spear lands. A bright rim, a soft fill,
              and a moving pattern: a turning ring of dashes in a circle, chevrons running along a lane.
              Parameters (set from C++ for each kind of mark):
                Color  the fill          Glow   the rim and the pattern
                Round  1 a circle, 0 a bar (the Plane's X is the lane's length)
                Fill   how strong the fill is (0 to 1)
                Pulse  how fast it beats (0 does not beat)
                Spin   how fast the pattern moves (turns a second in a circle; lengths a second along a lane)
                Reps   how many chevrons along a lane
                Arc    the share of a circle that is drawn, around the Plane's X (1 the whole circle; a third for the
                       brute's sweep)

Makes only what is missing. Delete the asset to make it again.
Run with the editor closed: ue.py py Tools/Art/make_effect_materials.py
"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import unreal

DEST = "/Game/Level"
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
made = []


def node(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def link(a, a_out, b, b_in):
    if not MEL.connect_material_expressions(a, a_out, b, b_in):
        fail("could not connect %s.%s to %s.%s" % (a.get_name(), a_out, b.get_name(), b_in))


def scalar(mat, name, value, x, y):
    param = node(mat, unreal.MaterialExpressionScalarParameter, x, y)
    param.set_editor_property("parameter_name", name)
    param.set_editor_property("default_value", value)
    return param


def vector(mat, name, value, x, y):
    param = node(mat, unreal.MaterialExpressionVectorParameter, x, y)
    param.set_editor_property("parameter_name", name)
    param.set_editor_property("default_value", unreal.LinearColor(value[0], value[1], value[2], 1.0))
    return param


def custom(mat, code, names, x, y):
    block = node(mat, unreal.MaterialExpressionCustom, x, y)
    block.set_editor_property("code", code)
    block.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs = []
    for name in names:
        one = unreal.CustomInput()
        one.set_editor_property("input_name", name)
        inputs.append(one)
    block.set_editor_property("inputs", inputs)
    return block


def new_material(name):
    path = DEST + "/" + name
    if EAL.does_asset_exist(path):
        out("already there:", path)
        made.append(unreal.load_asset(path))
        return None
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    # Drawn as instanced meshes: without this flag a packaged game draws the gray default instead.
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    return mat


def finish(mat, block):
    rgb = node(mat, unreal.MaterialExpressionComponentMask, 300, 0)
    for channel, on in (("r", True), ("g", True), ("b", True), ("a", False)):
        rgb.set_editor_property(channel, on)
    link(block, "", rgb, "")
    alpha = node(mat, unreal.MaterialExpressionComponentMask, 300, 200)
    for channel, on in (("r", False), ("g", False), ("b", False), ("a", True)):
        alpha.set_editor_property(channel, on)
    link(block, "", alpha, "")
    if not MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        fail("could not connect the emissive color")
    if not MEL.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY):
        fail("could not connect the opacity")
    MEL.recompile_material(mat)
    made.append(mat)
    out("made", mat.get_path_name())


FIRE = """
float flick = 0.85 + 0.15 * sin(T * 23.0 + UV.x * 40.0) * sin(T * 17.0 + UV.y * 31.0);
float beat = 0.82 + 0.18 * sin(T * Pulse);
float core = saturate(1.0 - F);
float heat = saturate(core * core * 1.25 * beat * flick - UV.y * 0.6);
float3 c = lerp(float3(0.45, 0.01, 0.0), float3(1.0, 0.22, 0.0), saturate(heat * 2.5));
c = lerp(c, float3(1.0, 0.6, 0.08), saturate(heat * 2.5 - 1.0));
c = lerp(c, float3(1.0, 0.95, 0.6), saturate(heat * 4.0 - 3.0));
float a = saturate(core * 2.4) * saturate(1.25 - UV.y * 0.95 + 0.6 * (flick - 0.85));
return float4(c * Bright * (0.55 + heat), a);
"""

MARK = """
float2 p = UV * 2.0 - 1.0;
float beat = 0.8 + 0.2 * sin(T * Pulse);
float rim = 0.0;
float mark = 0.0;
float fill = 0.0;
if (Round > 0.5)
{
    float r = length(p);
    float inside = 1.0 - smoothstep(0.985, 1.0, r);
    float side = 0.0;
    if (Arc < 0.999)
    {
        // Only a slice of the circle, around the Plane's X: its two straight sides glow like its rim.
        float open = Arc * 3.14159265 - abs(atan2(p.y, p.x));
        float across = open * max(r, 0.05);
        inside *= smoothstep(0.0, 0.02, across);
        side = (1.0 - smoothstep(0.03, 0.07, across)) * inside;
    }
    rim = max(smoothstep(0.86, 0.94, r), side) * inside;
    float ring = smoothstep(0.60, 0.63, r) * (1.0 - smoothstep(0.67, 0.70, r));
    float turn = atan2(p.y, p.x) / 6.2831853;
    float dash = smoothstep(0.35, 0.5, abs(frac(turn * 10.0 + T * Spin) - 0.5) * 2.0);
    float inner = smoothstep(0.30, 0.32, r) * (1.0 - smoothstep(0.34, 0.36, r));
    float back = smoothstep(0.35, 0.5, abs(frac(turn * 6.0 - T * Spin * 1.5) - 0.5) * 2.0);
    mark = (ring * dash + inner * back * 0.6) * inside;
    fill = inside * (0.25 + 0.75 * r * r);
}
else
{
    float2 q = abs(p);
    rim = max(smoothstep(0.6, 0.92, q.y), smoothstep(0.97, 1.0, q.x));
    float run = frac(UV.x * Reps - q.y * 0.35 - T * Spin);
    mark = smoothstep(0.0, 0.08, run) * (1.0 - smoothstep(0.18, 0.28, run)) * (1.0 - q.y);
    fill = 0.5;
}
float a = saturate(rim * 0.95 + mark * 0.7 + fill * Fill) * beat;
float3 c = lerp(Color, Glow, saturate(rim + mark * 0.8)) * (1.0 + rim * 0.5 + mark * 0.25);
return float4(c, a);
"""


def fire_bolt():
    mat = new_material("M_FireBolt")
    if not mat:
        return
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -700, -200)
    time = node(mat, unreal.MaterialExpressionTime, -700, -60)
    fresnel = node(mat, unreal.MaterialExpressionFresnel, -700, 60)
    fresnel.set_editor_property("exponent", 1.6)
    fresnel.set_editor_property("base_reflect_fraction", 0.0)
    pulse = scalar(mat, "Pulse", 9.0, -700, 220)
    bright = scalar(mat, "Bright", 1.5, -700, 320)
    block = custom(mat, FIRE, ["UV", "T", "F", "Pulse", "Bright"], -300, 0)
    link(uv, "", block, "UV")
    link(time, "", block, "T")
    link(fresnel, "", block, "F")
    link(pulse, "", block, "Pulse")
    link(bright, "", block, "Bright")

    # The pulse in its shape: every point moves out and in along its normal, a little.
    swell = scalar(mat, "Swell", 2.5, -700, 520)
    speed = node(mat, unreal.MaterialExpressionMultiply, -500, 440)
    link(time, "", speed, "A")
    link(pulse, "", speed, "B")
    wave = node(mat, unreal.MaterialExpressionSine, -340, 440)
    wave.set_editor_property("period", 6.2831853)
    link(speed, "", wave, "")
    amount = node(mat, unreal.MaterialExpressionMultiply, -180, 480)
    link(wave, "", amount, "A")
    link(swell, "", amount, "B")
    normal = node(mat, unreal.MaterialExpressionVertexNormalWS, -340, 600)
    push = node(mat, unreal.MaterialExpressionMultiply, 0, 540)
    link(normal, "", push, "A")
    link(amount, "", push, "B")
    if not MEL.connect_material_property(push, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET):
        fail("could not connect the world position offset")
    finish(mat, block)


def ground_mark():
    mat = new_material("M_GroundMark")
    if not mat:
        return
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -700, -300)
    time = node(mat, unreal.MaterialExpressionTime, -700, -200)
    color = vector(mat, "Color", (0.45, 0.02, 0.03), -700, -100)
    glow = vector(mat, "Glow", (1.0, 0.35, 0.08), -700, 80)
    names = ["UV", "T", "Color", "Glow", "Round", "Fill", "Pulse", "Spin", "Reps", "Arc"]
    block = custom(mat, MARK, names, -300, 0)
    link(uv, "", block, "UV")
    link(time, "", block, "T")
    link(color, "", block, "Color")
    link(glow, "", block, "Glow")
    y = 260
    for name, value in (("Round", 1.0), ("Fill", 0.4), ("Pulse", 6.0), ("Spin", 0.25), ("Reps", 6.0), ("Arc", 1.0)):
        link(scalar(mat, name, value, -700, y), "", block, name)
        y += 100
    finish(mat, block)


run_step("fire bolt", fire_bolt)
run_step("ground mark", ground_mark)
save_if_clean(*made)
