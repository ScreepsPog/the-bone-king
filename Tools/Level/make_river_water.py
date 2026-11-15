"""Headless Unreal: makes the river's water one smooth mesh with moving water on it.

- The river is forty square pieces in a stepped line. They stay (they are what stops feet), and no longer draw flat
  water of their own. This reads their middles (and the bridges', where the river runs under them), draws one smooth
  curve through them, and makes a ribbon as wide as the pieces along it: /Game/Level/SM_River_Water. Its texture
  coordinates run across the river (U) and along it (V), so the water flows the way the river goes.
- /Game/Level/M_River_Water: dark, cold water. Two sizes of ripple (a normal map made by
  Tools/Art/make_water_normal.py, /Game/Level/T_Water_Normal) slide downstream at different speeds over a slow swell;
  the sides of ripples show the night sky and those that lean toward the moon glint; greener shallows and a muddy
  bank at each edge. Opaque, one small texture read three times: cheap in a browser.
- One Static Mesh Actor, "River Water", is put in the level (or its mesh refreshed), and the level is saved.
  Nothing else in the level is touched.

Run again after moving river pieces. With the editor closed: ue.py py --render Tools/Level/make_river_water.py"""
from ue_bp import *  # out, fail, run_step
import math
import os
import unreal

MAP = "/Game/Maps/Outskirts"
FOLDER = "/Game/Level"
MESH = "SM_River_Water"
MATERIAL = "M_River_Water"
LABEL = "River Water"
# The water lies this far over the pieces' own height: just over the ground (which is 1.5 up), and under the bridges.
LIFT = 3.5
# One point of the curve every this far, and this much past each end.
STEP = 75.0
OVERRUN = 260.0

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
state = {}


def of_class(name):
    return [a for a in actors.get_all_level_actors() if a.get_class().get_name() == name]


def node(mat, cls, x, y, **props):
    made = MEL.create_material_expression(mat, cls, x, y)
    for key, value in props.items():
        made.set_editor_property(key, value)
    return made


def link(source, target, pin, output=""):
    if not MEL.connect_material_expressions(source, output, target, pin):
        fail("could not connect to " + pin)


def constant(mat, value, x, y):
    return node(mat, unreal.MaterialExpressionConstant, x, y, r=value)


def color(mat, rgb, x, y):
    return node(mat, unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def binary(mat, cls, a, b, x, y, a_out="", b_out=""):
    made = node(mat, cls, x, y)
    link(a, made, "A", a_out)
    link(b, made, "B", b_out)
    return made


TEXTURE_SOURCE = "C:/Dev/WebTesting/SourceArt/Level/T_Water_Normal.png"
TEXTURE = "T_Water_Normal"


def water_normal():
    """The rippled-water normal map made by Tools/Art/make_water_normal.py, imported once."""
    path = FOLDER + "/" + TEXTURE
    if not os.path.isfile(TEXTURE_SOURCE):
        fail("missing " + TEXTURE_SOURCE + " (run py Tools/Art/make_water_normal.py)")
        return None
    task = unreal.AssetImportTask()
    task.filename = TEXTURE_SOURCE
    task.destination_path = FOLDER
    task.destination_name = TEXTURE
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(path)
    if not texture:
        fail("did not import " + TEXTURE)
        return None
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    texture.set_editor_property("srgb", False)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
    EAL.save_loaded_asset(texture, False)
    out("water normal map:", path)
    return texture


def material():
    path = FOLDER + "/" + MATERIAL
    texture = water_normal()
    if not texture:
        return
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    mat = tools.create_asset(MATERIAL, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("two_sided", True)
    M = unreal.MaterialExpressionMultiply
    A = unreal.MaterialExpressionAdd
    coords = node(mat, unreal.MaterialExpressionTextureCoordinate, -2600, 0)
    split = node(mat, unreal.MaterialExpressionComponentMask, -2400, 0, r=True, g=True, b=False, a=False)
    link(coords, split, "")
    u = node(mat, unreal.MaterialExpressionComponentMask, -2200, -80, r=True, g=False, b=False, a=False)
    link(split, u, "")
    time = node(mat, unreal.MaterialExpressionTime, -2400, 300)

    def ripples(scale, drift, offset, x, y):
        """The normal map, repeated scale times across and along the river, sliding by drift (per second) downstream."""
        scaled = binary(mat, M, split, node(mat, unreal.MaterialExpressionConstant2Vector, x - 250, y + 80, r=scale[0], g=scale[1]), x, y)
        slide = binary(mat, M, time, node(mat, unreal.MaterialExpressionConstant2Vector, x - 250, y + 220, r=drift[0], g=drift[1]), x, y + 160)
        moved = binary(mat, A, binary(mat, A, scaled, slide, x + 200, y), node(mat, unreal.MaterialExpressionConstant2Vector, x + 200, y + 160, r=offset[0], g=offset[1]), x + 400, y)
        sample = node(mat, unreal.MaterialExpressionTextureSample, x + 600, y, texture=texture, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        link(moved, sample, "UVs")
        return sample

    # Two sizes of ripple sliding downstream at different speeds, so the pattern never repeats to the eye; and a
    # slow, wide swell under them. V runs along the river and grows downstream, so a ripple slides toward larger V.
    small = ripples((2.3, 2.1), (0.017, 0.13), (0.0, 0.0), -1900, 400)
    large = ripples((1.1, 0.9), (-0.011, 0.07), (0.37, 0.61), -1900, 900)
    swell = ripples((0.45, 0.22), (0.003, 0.02), (0.11, 0.83), -1900, 1400)

    # The bank: 0 in the middle of the river, 1 at either edge, rising late.
    centered = binary(mat, unreal.MaterialExpressionSubtract, u, constant(mat, 0.5, -2100, -260), -1900, -200)
    apart = node(mat, unreal.MaterialExpressionAbs, -1700, -200)
    link(centered, apart, "")
    bank = node(mat, unreal.MaterialExpressionPower, -1300, -200)
    link(binary(mat, M, apart, constant(mat, 2.0, -1700, -80), -1500, -200), bank, "Base")
    link(constant(mat, 8.0, -1500, -60), bank, "Exponent")
    clear = node(mat, unreal.MaterialExpressionOneMinus, -1100, -200)
    link(bank, clear, "")

    # The surface: both ripples' lean added, calmer toward the banks, then made a unit normal.
    def lean_of(sample, x, y):
        made = node(mat, unreal.MaterialExpressionComponentMask, x, y, r=True, g=True, b=False, a=False)
        link(sample, made, "", "RGB")
        return made

    lean = binary(mat, A, binary(mat, M, lean_of(small, -1000, 400), constant(mat, 0.22, -1000, 560), -800, 400),
                  binary(mat, M, lean_of(large, -1000, 900), constant(mat, 0.38, -1000, 1060), -800, 900), -600, 600)
    lean = binary(mat, A, lean, binary(mat, M, lean_of(swell, -1000, 1400), constant(mat, 0.3, -1000, 1560), -800, 1400), -400, 600)
    lean = binary(mat, M, lean, clear, -200, 600)
    normal = node(mat, unreal.MaterialExpressionAppendVector, 0, 600)
    link(lean, normal, "A")
    link(constant(mat, 1.0, -200, 760), normal, "B")
    unit = node(mat, unreal.MaterialExpressionNormalize, 200, 600)
    link(normal, unit, "VectorInput")
    MEL.connect_material_property(unit, "", unreal.MaterialProperty.MP_NORMAL)
    up = node(mat, unreal.MaterialExpressionComponentMask, 400, 760, r=False, g=False, b=True, a=False)
    link(unit, up, "")
    # How far the surface leans from flat here: 0 on still water, toward 1 on the side of a ripple.
    tilt = node(mat, unreal.MaterialExpressionSaturate, 800, 760)
    link(binary(mat, M, node_one_minus(mat, up, 600, 760), constant(mat, 22.0, 600, 900), 700, 760), tilt, "")

    # Dark, cold water, a little greener in the shallows by the bank; wet mud at the very edge.
    shallow = node(mat, unreal.MaterialExpressionPower, -900, -500)
    link(binary(mat, M, apart, constant(mat, 2.0, -1300, -560), -1100, -500), shallow, "Base")
    link(constant(mat, 2.5, -1100, -380), shallow, "Exponent")
    water = node(mat, unreal.MaterialExpressionLinearInterpolate, 400, -700)
    link(color(mat, (0.006, 0.016, 0.03), 100, -820), water, "A")
    link(color(mat, (0.014, 0.034, 0.038), 100, -680), water, "B")
    link(shallow, water, "Alpha")
    base = node(mat, unreal.MaterialExpressionLinearInterpolate, 700, -700)
    link(water, base, "A")
    link(color(mat, (0.028, 0.024, 0.018), 400, -540), base, "B")
    link(bank, base, "Alpha")
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = node(mat, unreal.MaterialExpressionLinearInterpolate, 700, -300)
    link(constant(mat, 0.2, 400, -360), rough, "A")
    link(constant(mat, 0.8, 400, -280), rough, "B")
    link(bank, rough, "Alpha")
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(constant(mat, 0.35, 700, -140), "", unreal.MaterialProperty.MP_SPECULAR)

    # The level has no reflections, so the sky's is put in by hand: the sides of ripples show the night sky, and
    # where the surface leans toward the moon it glints.
    sky = binary(mat, M, binary(mat, M, tilt, clear, 1000, 760), color(mat, (0.02, 0.038, 0.075), 1000, 900), 1200, 760)
    toward = node(mat, unreal.MaterialExpressionDotProduct, 600, 1100)
    link(unit, toward, "A")
    link(color(mat, (0.19, -0.115, 0.975), 400, 1200), toward, "B")
    glint = node(mat, unreal.MaterialExpressionPower, 1000, 1100)
    lit = node(mat, unreal.MaterialExpressionSaturate, 800, 1100)
    link(toward, lit, "")
    link(lit, glint, "Base")
    link(constant(mat, 60.0, 800, 1240), glint, "Exponent")
    moon = binary(mat, M, binary(mat, M, glint, clear, 1200, 1100), color(mat, (0.035, 0.05, 0.075), 1200, 1240), 1400, 1100)
    MEL.connect_material_property(binary(mat, A, sky, moon, 1600, 900), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    try:
        MEL.recompile_material(mat)
    except RuntimeError as error:
        out("not compiled here:", str(error)[:90])
    EAL.save_loaded_asset(mat, False)
    out("made:", path, "expressions", MEL.get_num_material_expressions(mat))
    state["material"] = mat


def node_one_minus(mat, source, x, y):
    made = node(mat, unreal.MaterialExpressionOneMinus, x, y)
    link(source, made, "")
    return made


def curve(points):
    """Points along a smooth curve through the given points (Catmull-Rom), about STEP apart, with their directions."""
    padded = [points[0]] + points + [points[-1]]
    made = []
    for index in range(1, len(padded) - 2):
        p0, p1, p2, p3 = padded[index - 1], padded[index], padded[index + 1], padded[index + 2]
        pieces = max(int(math.hypot(p2[0] - p1[0], p2[1] - p1[1]) / STEP), 1)
        for step in range(pieces):
            t = step / pieces
            made.append(tuple(0.5 * ((2 * p1[a]) + (-p0[a] + p2[a]) * t + (2 * p0[a] - 5 * p1[a] + 4 * p2[a] - p3[a]) * t * t
                                     + (-p0[a] + 3 * p1[a] - 3 * p2[a] + p3[a]) * t * t * t) for a in (0, 1)))
    made.append(points[-1])
    return made


def mesh():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    points = []
    width = 0.0
    height = 0.0
    for piece in of_class("LevelPieceActor"):
        kind = str(piece.get_editor_property("Kind")).upper()
        if "RIVER" in kind or "BRIDGE" in kind:
            where = piece.get_actor_location()
            # A piece turned sideways (the director puts them beside the bridges) is not on the river's line:
            # the curve ran through them and zigzagged at each bridge.
            turn = abs(piece.get_actor_rotation().yaw) % 180.0
            if 45.0 < turn < 135.0:
                continue
            points.append((where.x, where.y))
            if "RIVER" in kind:
                width = max(width, piece.get_editor_property("Size").y * abs(piece.get_actor_scale3d().y))
                height = max(height, where.z)
    if len(points) < 3:
        fail("no river in the level")
        return
    points.sort()
    # Bridges are not on the river's line (they are wider than a piece is long): a bridge's point is put between its neighbors.
    line = curve(points)
    # A little past each end, straight on.
    first = (line[0][0] - (line[1][0] - line[0][0]) / STEP * OVERRUN, line[0][1] - (line[1][1] - line[0][1]) / STEP * OVERRUN)
    last = (line[-1][0] + (line[-1][0] - line[-2][0]) / STEP * OVERRUN, line[-1][1] + (line[-1][1] - line[-2][1]) / STEP * OVERRUN)
    line = [first] + line + [last]
    out("river: pieces and bridges", len(points), "curve points", len(line), "width", round(width), "from", [round(v) for v in line[0]], "to", [round(v) for v in line[-1]])

    path = FOLDER + "/" + MESH
    asset = unreal.load_asset(path) if EAL.does_asset_exist(path) else EAL.duplicate_asset("/Engine/BasicShapes/Plane", path)
    if not asset:
        fail("could not make " + path)
        return
    description = unreal.StaticMesh.create_static_mesh_description(asset)
    group = description.create_polygon_group()
    rows = []
    travelled = 0.0
    for index, point in enumerate(line):
        before = line[max(index - 1, 0)]
        after = line[min(index + 1, len(line) - 1)]
        along = (after[0] - before[0], after[1] - before[1])
        size = math.hypot(along[0], along[1]) or 1.0
        side = (-along[1] / size, along[0] / size)
        if index:
            travelled += math.hypot(point[0] - line[index - 1][0], point[1] - line[index - 1][1])
        row = []
        for across in (0.0, 0.5, 1.0):
            vertex = description.create_vertex()
            offset = (across - 0.5) * width
            description.set_vertex_position(vertex, unreal.Vector(point[0] + side[0] * offset, point[1] + side[1] * offset, 0.0))
            row.append((vertex, unreal.Vector2D(across, travelled / width)))
        rows.append(row)
    triangles = 0
    for index in range(len(rows) - 1):
        for column in range(2):
            a, b = rows[index][column], rows[index][column + 1]
            c, d = rows[index + 1][column], rows[index + 1][column + 1]
            # Wound to face up (the first try faced down, and nothing showed from above).
            for corners in ((a, b, c), (b, d, c)):
                instances = []
                for vertex, uv in corners:
                    instance = description.create_vertex_instance(vertex)
                    description.set_vertex_instance_uv(instance, uv, 0)
                    instances.append(instance)
                description.create_triangle(group, instances)
                triangles += 1
    asset.build_from_static_mesh_descriptions([description], False)
    # Normals and tangents are worked out from the triangles (the description carries none), and it has no collision.
    library = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) if hasattr(unreal, "StaticMeshEditorSubsystem") else None
    library = library or getattr(unreal, "EditorStaticMeshLibrary", None)
    if library:
        settings = library.get_lod_build_settings(asset, 0)
        settings.set_editor_property("recompute_normals", True)
        settings.set_editor_property("recompute_tangents", True)
        settings.set_editor_property("generate_lightmap_u_vs", False)
        library.set_lod_build_settings(asset, 0, settings)
        library.remove_collisions(asset)
    else:
        out("no static mesh editing library here: build settings left as the plane's")
    if state.get("material"):
        # Every slot: building from a description can leave the plane's own slot beside a new one.
        slots = len(asset.get_editor_property("static_materials"))
        for slot in range(max(slots, 1)):
            asset.set_material(slot, state["material"])
        out("material slots:", slots)
    bounds = asset.get_bounding_box()
    out("mesh:", path, "triangles", triangles, "from", [round(bounds.min.x), round(bounds.min.y), round(bounds.min.z)], "to", [round(bounds.max.x), round(bounds.max.y), round(bounds.max.z)])
    if bounds.max.x - bounds.min.x < 1000.0:
        fail("the mesh was not built from the river's line")
        return
    EAL.save_loaded_asset(asset, False)

    placed = [a for a in of_class("StaticMeshActor") if a.get_actor_label() == LABEL]
    if placed:
        actor = placed[0]
        out("already there:", LABEL)
    else:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, height + LIFT))
        actor.set_actor_label(LABEL)
        actor.set_folder_path("River")
    component = actor.static_mesh_component
    component.set_static_mesh(asset)
    component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_cast_shadow(False)
    if state.get("material"):
        for slot in range(component.get_num_materials()):
            component.set_material(slot, state["material"])
    actor.set_actor_location(unreal.Vector(0.0, 0.0, height + LIFT), False, False)
    out("river water at height", height + LIFT)
    out("saved level:", levels.save_current_level())


run_step("material", material)
run_step("mesh", mesh)
