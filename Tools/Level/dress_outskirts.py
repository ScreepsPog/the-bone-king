"""Headless Unreal: makes the Outskirts level a level like any other, with nothing laid out when the game starts.
Run after Tools/Level/make_outskirts_editable.py.

1. Materials in /Game/Level: M_LevelPiece (a texture laid over the world and a tint) with one instance for each kind
   of piece (MI_Piece_Path, MI_Piece_River, ...), M_ShrineRange (the pulsing ring) and M_PostArea (a post's ground).
2. Every house mesh gets a solid box of its own, so nothing has to make walls for buildings when the game starts.
3. The shrine, every post, the commander and every patrol road become actors (the layout actor's Place Systems As
   Actors), and the player start goes to where the wizard begins.
4. Every lamp becomes the lamp post model. Every house box that is left becomes a house mesh, scaled toward the size
   of its box; a box whose house would stand on a road, the river, another building or a post's ground is removed.
5. The shrine is moved to the nearest spot clear of roads, lamps and buildings.
6. Each post gets the ground its group needs, moved where it has to be so that ground is open and no two posts' grounds meet.

Makes only what is missing; posts and the shrine are checked again on every run. Needs a renderer for the materials:
after `ue.py py Tools/Level/dress_outskirts.py` has written its runner, run
  UnrealEditor-Cmd.exe <project> -run=pythonscript -script=%TEMP%/ue_claude_plugin/dress_outskirts.py
      -AllowCommandletRendering -RenderOffscreen -unattended -nop4 -nosplash"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import math
import unreal

MAP = "/Game/Maps/Outskirts"
LEVEL = "/Game/Level"
# name, tint, tile size (units one repeat of the texture covers), roughness, glow
PIECES = [
    ("House", (0.2, 0.17, 0.15), 400.0, 0.9, 0.0),
    ("Medical", (0.62, 0.58, 0.52), 400.0, 0.9, 0.0),
    ("Dock", (0.25, 0.14, 0.06), 300.0, 0.8, 0.0),
    ("Coal", (0.02, 0.02, 0.025), 300.0, 0.6, 0.0),
    ("Tree", (0.13, 0.08, 0.04), 200.0, 0.9, 0.0),
    ("River", (0.03, 0.16, 0.5), 800.0, 0.15, 0.0),
    ("Bridge", (0.3, 0.2, 0.1), 300.0, 0.8, 0.0),
    ("Path", (0.24, 0.17, 0.1), 500.0, 0.95, 0.0),
    ("Farmland", (0.2, 0.36, 0.06), 600.0, 0.95, 0.0),
    ("Graveyard", (0.1, 0.13, 0.12), 600.0, 0.95, 0.0),
    ("Lamp", (0.03, 0.03, 0.03), 200.0, 0.5, 0.0),
    ("TreeTop", (0.04, 0.17, 0.07), 300.0, 0.9, 0.0),
    ("LampGlow", (1.0, 0.8, 0.15), 200.0, 0.5, 3.0),
]
HOUSES = ["Peasant_Cottage", "Burned_Cottage", "Barn", "Twostory_House"]
BODY = 0.85
# A house is scaled toward its box, but not past these: its door is 230 tall at 1.
SCALE_LEAST = 0.5
SCALE_MOST = 1.1
GAP = 10.0
LIFT = 2.0
SHRINE_RANGE = 700.0
SHRINE_GAP = 60.0
POST_GAP = 120.0

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
made = []


def node(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def link(a, a_out, b, b_in):
    if not MEL.connect_material_expressions(a, a_out, b, b_in):
        fail("could not connect %s.%s to %s.%s" % (a.get_name(), a_out, b.get_name(), b_in))


def compile_now(mat):
    try:
        MEL.recompile_material(mat)
    except RuntimeError as error:
        out("not compiled here:", str(error)[:80])


def piece_material():
    path = LEVEL + "/M_LevelPiece"
    if EAL.does_asset_exist(path):
        out("already there:", path)
        mat = unreal.load_asset(path)
    else:
        mat = tools.create_asset("M_LevelPiece", LEVEL, unreal.Material, unreal.MaterialFactoryNew())
        # The texture is laid over the world, not over the piece: a road made of many pieces, or one scaled, tiles as one.
        world = node(mat, unreal.MaterialExpressionWorldPosition, -1500, 0)
        mask = node(mat, unreal.MaterialExpressionComponentMask, -1300, 0)
        mask.set_editor_property("r", True)
        mask.set_editor_property("g", True)
        mask.set_editor_property("b", False)
        mask.set_editor_property("a", False)
        link(world, "", mask, "")
        tile = node(mat, unreal.MaterialExpressionScalarParameter, -1300, 160)
        tile.set_editor_property("parameter_name", "Tile Size")
        tile.set_editor_property("default_value", 400.0)
        uv = node(mat, unreal.MaterialExpressionDivide, -1100, 0)
        link(mask, "", uv, "A")
        link(tile, "", uv, "B")
        sample = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
        sample.set_editor_property("parameter_name", "Texture")
        sample.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
        sample.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        link(uv, "", sample, "UVs")
        tint = node(mat, unreal.MaterialExpressionVectorParameter, -900, 260)
        tint.set_editor_property("parameter_name", "Tint")
        tint.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
        color = node(mat, unreal.MaterialExpressionMultiply, -600, 0)
        link(sample, "RGB", color, "A")
        link(tint, "", color, "B")
        MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = node(mat, unreal.MaterialExpressionScalarParameter, -600, 260)
        rough.set_editor_property("parameter_name", "Roughness")
        rough.set_editor_property("default_value", 0.9)
        MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        glow = node(mat, unreal.MaterialExpressionScalarParameter, -900, 480)
        glow.set_editor_property("parameter_name", "Glow")
        glow.set_editor_property("default_value", 0.0)
        lit = node(mat, unreal.MaterialExpressionMultiply, -600, 480)
        link(color, "", lit, "A")
        link(glow, "", lit, "B")
        MEL.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        compile_now(mat)
        out("made:", path)
    made.append(mat)
    for name, tint, tile, rough, glow in PIECES:
        path = LEVEL + "/MI_Piece_" + name
        if EAL.does_asset_exist(path):
            made.append(unreal.load_asset(path))
            continue
        inst = tools.create_asset("MI_Piece_" + name, LEVEL, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(inst, mat)
        MEL.set_material_instance_vector_parameter_value(inst, "Tint", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
        MEL.set_material_instance_scalar_parameter_value(inst, "Tile Size", tile)
        MEL.set_material_instance_scalar_parameter_value(inst, "Roughness", rough)
        MEL.set_material_instance_scalar_parameter_value(inst, "Glow", glow)
        made.append(inst)
        out("made:", path)


def ring_material(name, color, code, timed):
    path = LEVEL + "/" + name
    if EAL.does_asset_exist(path):
        out("already there:", path)
        made.append(unreal.load_asset(path))
        return
    mat = tools.create_asset(name, LEVEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    tint = node(mat, unreal.MaterialExpressionVectorParameter, -700, 0)
    tint.set_editor_property("parameter_name", "Color")
    tint.set_editor_property("default_value", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    MEL.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    coords = node(mat, unreal.MaterialExpressionTextureCoordinate, -1100, 260)
    custom = node(mat, unreal.MaterialExpressionCustom, -700, 260)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    inputs = [unreal.CustomInput()]
    inputs[0].set_editor_property("input_name", "UV")
    if timed:
        inputs.append(unreal.CustomInput())
        inputs[1].set_editor_property("input_name", "T")
    custom.set_editor_property("inputs", inputs)
    custom.set_editor_property("code", code)
    link(coords, "", custom, "UV")
    if timed:
        clock = node(mat, unreal.MaterialExpressionTime, -1100, 420)
        link(clock, "", custom, "T")
    MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_OPACITY)
    compile_now(mat)
    made.append(mat)
    out("made:", path)


def materials():
    piece_material()
    # The shrine's range: a bright rim at the range, and rings that run outward to it, as if the shrine were breathing.
    ring_material("M_ShrineRange", (0.5, 1.9, 1.6),
                  "float d = length(UV - 0.5) * 2.0;\n"
                  "float rim = saturate(1.0 - abs(d - 0.965) / 0.03);\n"
                  "float wave = frac(d * 2.5 - T * 0.35);\n"
                  "float ripple = pow(wave, 6.0) * 0.4 * d * step(d, 0.965);\n"
                  "return saturate(rim * 0.85 + ripple);", True)
    # A post's ground: a faint disc with a clear rim.
    ring_material("M_PostArea", (1.0, 0.22, 0.12),
                  "float d = length(UV - 0.5) * 2.0;\n"
                  "float rim = saturate(1.0 - abs(d - 0.97) / 0.03);\n"
                  "return saturate(rim * 0.8 + 0.12 * step(d, 0.97));", False)


def solid_buildings():
    for name in HOUSES + ["Shrine"]:
        mesh = unreal.load_asset("/Game/Buildings/SM_" + name)
        if unreal.CrowdRenderer.make_mesh_solid(mesh):
            out("solid:", name)
        else:
            fail("could not make " + name + " solid")
        made.append(mesh)


def of_class(name):
    return [a for a in actors.get_all_level_actors() if a.get_class().get_name() == name]


def kind_of(piece):
    return str(piece.get_editor_property("kind")).split(".")[-1].split(":")[0].upper()


def rect_of(piece):
    size = piece.get_editor_property("size")
    scale = piece.get_actor_scale3d()
    yaw = math.radians(piece.get_actor_rotation().yaw)
    hx, hy = size.x * abs(scale.x) / 2, size.y * abs(scale.y) / 2
    ex = hx * abs(math.cos(yaw)) + hy * abs(math.sin(yaw))
    ey = hx * abs(math.sin(yaw)) + hy * abs(math.cos(yaw))
    at = piece.get_actor_location()
    return (at.x - ex, at.y - ey, at.x + ex, at.y + ey)


def mesh_rect(actor, share=BODY):
    """A placed mesh's box on the ground, drawn in by share as the game draws a building's in."""
    origin, extent = actor.get_actor_bounds(False)
    return (origin.x - extent.x * share, origin.y - extent.y * share, origin.x + extent.x * share, origin.y + extent.y * share)


def touches(a, b, gap):
    return a[0] < b[2] + gap and b[0] < a[2] + gap and a[1] < b[3] + gap and b[1] < a[3] + gap


def circle_touches(x, y, radius, rect):
    nx, ny = min(max(x, rect[0]), rect[2]), min(max(y, rect[1]), rect[3])
    return (x - nx) ** 2 + (y - ny) ** 2 < radius ** 2


def building_actors():
    return [a for a in of_class("StaticMeshActor")
            if a.static_mesh_component.static_mesh and str(a.static_mesh_component.static_mesh.get_path_name()).startswith("/Game/Buildings/")]


def lamp_actors():
    return [a for a in of_class("StaticMeshActor")
            if a.static_mesh_component.static_mesh and "SM_Lamp_Post" in str(a.static_mesh_component.static_mesh.get_path_name())]


def place_mesh(mesh, x, y, z, yaw, scale, label, folder):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0.0, 0.0, yaw))
    if not actor:
        fail("could not place " + label)
        return None
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    return actor


def lamps(ground):
    mesh = unreal.load_asset("/Game/Props/SM_Lamp_Post")
    if not mesh:
        fail("no lamp post model: import it first (ART_SET=Props, Tools/Art/import_building.py)")
        return
    count = len(lamp_actors())
    for piece in [p for p in of_class("LevelPieceActor") if kind_of(p) == "LAMP"]:
        at = piece.get_actor_location()
        count += 1
        # Each turned its own way, so twenty of one model do not look stamped.
        if place_mesh(mesh, at.x, at.y, ground + LIFT, (count * 77) % 360, 1.0, "Lamp Post %02d" % count, "Lamps"):
            actors.destroy_actor(piece)
    out("lamp posts:", len(lamp_actors()), "models,", len([p for p in of_class("LevelPieceActor") if kind_of(p) == "LAMP"]), "plain ones left")


def houses(ground, posts):
    meshes = []
    for name in HOUSES:
        mesh = unreal.load_asset("/Game/Buildings/SM_" + name)
        box = mesh.get_bounding_box()
        meshes.append((name, mesh, (box.max.x - box.min.x) / 2, (box.max.y - box.min.y) / 2))
    boxes = sorted([p for p in of_class("LevelPieceActor") if kind_of(p) == "HOUSE"], key=lambda p: (p.get_actor_location().x, p.get_actor_location().y))
    if not boxes:
        out("already there:", len(building_actors()), "house meshes, no boxes left")
        return
    solid = {p: rect_of(p) for p in of_class("LevelPieceActor") if kind_of(p) not in ("FARMLAND", "GRAVEYARD", "LAMP")}
    taken = [mesh_rect(a) for a in building_actors()]
    start = [(p.x, p.y) for p in unreal.CrowdRenderer.get_layout_key_points()][0]
    count = len(taken)
    placed = 0
    dropped = 0
    scales = []
    for index, box_actor in enumerate(boxes):
        at = box_actor.get_actor_location()
        own = solid.pop(box_actor)
        half = ((own[2] - own[0]) / 2, (own[3] - own[1]) / 2)
        best = None
        # Whichever model and way round fills the box best; a different model first each time, for variety.
        for turn in range(len(meshes) * 2):
            name, mesh, hx, hy = meshes[(index + turn // 2) % len(meshes)]
            yaw = 90.0 if turn % 2 else 0.0
            if yaw:
                hx, hy = hy, hx
            fit = min(half[0] / (hx * BODY), half[1] / (hy * BODY))
            scale = min(max(fit, SCALE_LEAST), SCALE_MOST)
            fill = (hx * hy * scale * scale * BODY * BODY) / (half[0] * half[1])
            # How far from filling the box exactly; one that sticks out of its box is a poor choice; a first choice wins ties.
            over = max(hx * BODY * scale / half[0], hy * BODY * scale / half[1])
            score = abs(math.log(max(fill, 1e-6))) + 0.02 * turn + (3.0 * (over - 1.0) if over > 1.0 else 0.0)
            if not best or score < best[0]:
                best = (score, name, mesh, yaw, scale, hx, hy)
        score, name, mesh, yaw, scale, hx, hy = best
        ok = None
        for shrink in (1.0, 0.92, 0.85):
            used = max(scale * shrink, SCALE_LEAST)
            rect = (at.x - hx * BODY * used, at.y - hy * BODY * used, at.x + hx * BODY * used, at.y + hy * BODY * used)
            # A house no larger than its box stands where the box stood, which was clear already.
            within = rect[0] >= own[0] - 1.0 and rect[1] >= own[1] - 1.0 and rect[2] <= own[2] + 1.0 and rect[3] <= own[3] + 1.0
            blocked = not within and (any(touches(rect, other, GAP) for other in solid.values()) or any(touches(rect, other, GAP) for other in taken))
            # Never on a post's ground, on the shrine's, or where the wizard starts.
            blocked = blocked or any(circle_touches(x, y, radius + 40.0, rect) for x, y, radius in posts)
            blocked = blocked or circle_touches(start[0], start[1], 400.0, rect)
            if not blocked:
                ok = (used, rect)
                break
        actors.destroy_actor(box_actor)
        if not ok:
            dropped += 1
            continue
        used, rect = ok
        count += 1
        if place_mesh(mesh, at.x, at.y, ground + LIFT, yaw, used, "House %s %02d" % (name.replace("_", " "), count), "Buildings"):
            placed += 1
            taken.append(rect)
            scales.append(used)
    scales.sort()
    out("houses:", placed, "boxes became meshes,", dropped, "boxes removed (no room for a house there);", len(building_actors()), "house meshes in all")
    if scales:
        out("house scales: least", round(scales[0], 2), "middle", round(scales[len(scales) // 2], 2), "most", round(scales[-1], 2))


def blockers(include_shrine=True):
    """Everything a unit cannot walk through, as boxes on the ground."""
    rects = [rect_of(p) for p in of_class("LevelPieceActor") if kind_of(p) in ("HOUSE", "MEDICAL", "DOCK", "COAL", "TREE", "RIVER")]
    rects += [mesh_rect(a) for a in building_actors()]
    if include_shrine:
        rects += [mesh_rect(a) for a in of_class("ShrineActor")]
    return rects


def spiral(step, reach):
    """Offsets on a grid, nearest first."""
    cells = int(reach / step)
    spots = [(x * step, y * step) for x in range(-cells, cells + 1) for y in range(-cells, cells + 1)]
    return sorted(spots, key=lambda s: s[0] ** 2 + s[1] ** 2)


def shrine(posts):
    found = of_class("ShrineActor")
    if not found:
        fail("no shrine actor in the level")
        return None
    actor = found[0]
    actor.set_editor_property("range", SHRINE_RANGE)
    origin, extent = actor.get_actor_bounds(False)
    at = actor.get_actor_location()
    # The model's own footprint: the range ring is far wider and may lie over a road.
    box = actor.get_components_by_class(unreal.StaticMeshComponent)[0].static_mesh.get_bounding_box()
    hx, hy = (box.max.x - box.min.x) / 2, (box.max.y - box.min.y) / 2
    roads = [rect_of(p) for p in of_class("LevelPieceActor") if kind_of(p) in ("PATH", "BRIDGE")]
    # House boxes do not count: one that is in the shrine's way goes when the houses are placed.
    solid = [rect_of(p) for p in of_class("LevelPieceActor") if kind_of(p) in ("MEDICAL", "DOCK", "COAL", "TREE", "RIVER")]
    solid += [mesh_rect(a) for a in building_actors()]
    lamp_spots = [(a.get_actor_location().x, a.get_actor_location().y) for a in lamp_actors()]
    for dx, dy in spiral(50.0, 2500.0):
        x, y = at.x + dx, at.y + dy
        rect = (x - hx, y - hy, x + hx, y + hy)
        if any(touches(rect, other, SHRINE_GAP) for other in roads + solid):
            continue
        if any(rect[0] - 90.0 < lx < rect[2] + 90.0 and rect[1] - 90.0 < ly < rect[3] + 90.0 for lx, ly in lamp_spots):
            continue
        if any(circle_touches(px, py, radius + 60.0, rect) for px, py, radius in posts):
            continue
        # The wizard walks the road: it must pass through the range.
        if not any(circle_touches(x, y, SHRINE_RANGE - 150.0, road) for road in roads):
            continue
        actor.set_actor_location(unreal.Vector(x, y, at.z), False, False)
        out("shrine: moved", round(math.hypot(dx, dy)), "units to", round(x), round(y), "range", SHRINE_RANGE, "footprint", round(hx * 2), "by", round(hy * 2))
        return (x, y, SHRINE_RANGE)
    fail("no clear spot for the shrine within 2500 units")
    return None


def post_spots(x, y, count, radius):
    golden = 2.39996322972865332
    reach = radius * 0.8
    return [(x + math.cos(i * golden) * reach * math.sqrt(i / max(count - 1, 1)), y + math.sin(i * golden) * reach * math.sqrt(i / max(count - 1, 1))) for i in range(count)]


def inside(x, y, rect, margin):
    return rect[0] - margin < x < rect[2] + margin and rect[1] - margin < y < rect[3] + margin


def post_data():
    data = []
    for actor in sorted(of_class("EnemyPostActor"), key=lambda a: a.get_actor_label()):
        count = actor.get_editor_property("grunts") + actor.get_editor_property("archers") + actor.get_editor_property("brutes") + (1 if actor.get_editor_property("commander") else 0)
        data.append((actor, count))
    return data


def posts_fix(shrine_spot):
    solid = blockers()
    fixed = []
    start = [(p.x, p.y) for p in unreal.CrowdRenderer.get_layout_key_points()][0]
    moved = 0
    for actor, count in post_data():
        at = actor.get_actor_location()
        need = max(300.0, 115.0 * math.sqrt(max(count, 1)))
        done = False
        for dx, dy in spiral(100.0, 2000.0):
            x, y = at.x + dx, at.y + dy
            if any(inside(x, y, rect, 90.0) for rect in solid):
                continue
            # Its ground is open: nearly every place one of the group would stand is clear.
            spots = post_spots(x, y, count + 1, need)
            clear = sum(1 for sx, sy in spots if not any(inside(sx, sy, rect, 45.0) for rect in solid))
            if clear < 0.92 * len(spots):
                continue
            if any(math.hypot(x - fx, y - fy) < need + radius + POST_GAP for fx, fy, radius in fixed):
                continue
            if shrine_spot and math.hypot(x - shrine_spot[0], y - shrine_spot[1]) < need + 500.0:
                continue
            if math.hypot(x - start[0], y - start[1]) < need + 900.0:
                continue
            actor.set_actor_location(unreal.Vector(x, y, at.z), False, False)
            actor.set_editor_property("radius", need)
            fixed.append((x, y, need))
            if dx or dy:
                moved += 1
                out("post:", actor.get_actor_label(), "of", count, "moved", round(math.hypot(dx, dy)), "units; ground", round(need))
            done = True
            break
        if not done:
            fail("no open ground for " + actor.get_actor_label() + " of " + str(count))
    out("posts:", len(fixed), "with open ground of their own,", moved, "moved")
    return fixed


def make():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    found = of_class("CrowdRenderer")
    if not found:
        fail("no layout actor in " + MAP)
        return
    layout = found[0]
    ground = layout.get_actor_location().z
    if of_class("EnemyPostActor"):
        out("already there:", len(of_class("EnemyPostActor")), "posts,", len(of_class("PatrolRouteActor")), "patrol roads,", len(of_class("ShrineActor")), "shrine")
    else:
        layout.place_systems_as_actors()
        out("placed:", len(of_class("EnemyPostActor")), "posts,", len(of_class("PatrolRouteActor")), "patrol roads,", len(of_class("ShrineActor")), "shrine")
    lamps(ground)
    # The shrine first, then the houses around it. Houses keep off the ground the posts need where they now stand.
    wanted = [(a.get_actor_location().x, a.get_actor_location().y, max(300.0, 115.0 * math.sqrt(max(c, 1)))) for a, c in post_data()]
    shrine_spot = shrine(wanted)
    if shrine_spot:
        # As a post's ground is kept: no house on the shrine or pressed against it.
        wanted.append((shrine_spot[0], shrine_spot[1], 560.0))
    houses(ground, wanted)
    wanted.pop()
    posts_fix(shrine_spot)
    layout.refresh_placed_actors()
    names = ("LevelPieceActor", "StaticMeshActor", "ShrineActor", "EnemyPostActor", "PatrolRouteActor", "Landscape", "PlayerStart")
    out("actors by class:", {name: len(of_class(name)) for name in names})
    out("saved level:", levels.save_current_level())


run_step("materials", materials)
run_step("solid buildings", solid_buildings)
save_if_clean(*made)
run_step("level", make)
