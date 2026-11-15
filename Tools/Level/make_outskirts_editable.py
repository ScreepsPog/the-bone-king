"""Headless Unreal: makes the Outskirts level editable by hand. Run after Tools/Level/make_outskirts_map.py.

1. Every piece of the code-made layout (houses, trees, river, roads, fields, lamps) becomes its own Level Piece actor
   in the level, in folders by kind, and the layout actor is switched to use them.
2. Some of the house boxes are replaced by the house meshes of /Game/Buildings, where a mesh fits without touching a
   road, the river, another building or a place the game needs clear.
3. The floor becomes a flat landscape with four paint layers (Ground, Grass, Dirt, Stone), each a texture and a tint
   on the material instance /Game/Landscape/MI_Ground.

Makes only what is missing. The landscape's paint layers are merged by the renderer, so this needs one: after
`ue.py py Tools/Level/make_outskirts_editable.py` has written its runner (it fails there for want of a renderer), run
  UnrealEditor-Cmd.exe <project> -run=pythonscript -script=%TEMP%/ue_claude_plugin/make_outskirts_editable.py
      -AllowCommandletRendering -RenderOffscreen -unattended -nop4 -nosplash"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import unreal

MAP = "/Game/Maps/Outskirts"
FOLDER = "/Game/Landscape"
LAYERS = [  # name, tint
    ("Ground", (0.09, 0.08, 0.07)),
    ("Grass", (0.10, 0.20, 0.05)),
    ("Dirt", (0.24, 0.17, 0.10)),
    ("Stone", (0.30, 0.30, 0.30)),
]
# One quad is a meter; 5 by 4 components of 63 quads cover the level with room to spare.
QUAD = 100.0
COMPONENTS = (5, 4)
QUADS = 63
# The landscape lies this far above the layout actor, where the game's own ground was.
LIFT = 2.0
HOUSES = ["Peasant_Cottage", "Burned_Cottage", "Barn", "Twostory_House"]
MOST_HOUSES = 16
# A mesh blocks 85% of its footprint in the game; it is kept this far from anything else.
BODY = 0.85
GAP = 60.0
KEY_GAP = 350.0

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
made = []


def of_class(name):
    return [a for a in actors.get_all_level_actors() if a.get_class().get_name() == name]


def kind_of(piece):
    return str(piece.get_editor_property("kind")).split(".")[-1].split(":")[0].upper()


def rect_of(piece):
    """The square box around a piece: (min x, min y, max x, max y)."""
    import math
    size = piece.get_editor_property("size")
    scale = piece.get_actor_scale3d()
    yaw = math.radians(piece.get_actor_rotation().yaw)
    hx, hy = size.x * abs(scale.x) / 2, size.y * abs(scale.y) / 2
    ex = hx * abs(math.cos(yaw)) + hy * abs(math.sin(yaw))
    ey = hx * abs(math.sin(yaw)) + hy * abs(math.cos(yaw))
    at = piece.get_actor_location()
    return (at.x - ex, at.y - ey, at.x + ex, at.y + ey)


def touches(a, b, gap):
    return a[0] < b[2] + gap and b[0] < a[2] + gap and a[1] < b[3] + gap and b[1] < a[3] + gap


def pieces(layout):
    if of_class("LevelPieceActor"):
        out("already there:", len(of_class("LevelPieceActor")), "piece actors")
    else:
        layout.place_pieces_as_actors()
        out("placed:", len(of_class("LevelPieceActor")), "piece actors")
    if not layout.get_editor_property("use_placed_pieces"):
        layout.set_editor_property("use_placed_pieces", True)
    counts = {}
    for piece in of_class("LevelPieceActor"):
        counts[kind_of(piece)] = counts.get(kind_of(piece), 0) + 1
    out("pieces by kind:", counts)


def houses(layout):
    placed = [a for a in of_class("StaticMeshActor") if str(a.static_mesh_component.static_mesh.get_path_name()).startswith("/Game/Buildings/")]
    if placed:
        out("already there:", len(placed), "mesh houses")
        return
    meshes = []
    for name in HOUSES:
        mesh = unreal.load_asset("/Game/Buildings/SM_" + name)
        box = mesh.get_bounding_box()
        meshes.append((name, mesh, box))
    keys = [(p.x, p.y) for p in unreal.CrowdRenderer.get_layout_key_points()]
    all_pieces = of_class("LevelPieceActor")
    solid = [(p, rect_of(p)) for p in all_pieces if kind_of(p) not in ("FARMLAND", "GRAVEYARD")]
    boxes = sorted([p for p in all_pieces if kind_of(p) == "HOUSE"], key=lambda p: (p.get_actor_location().x, p.get_actor_location().y))
    taken = []
    count = 0
    ground = layout.get_actor_location().z
    for index, box_actor in enumerate(boxes):
        if count >= MOST_HOUSES:
            break
        at = box_actor.get_actor_location()
        # A different mesh each time, and whichever way round fits.
        choice = None
        for turn in range(len(meshes) * 2):
            name, mesh, bounds = meshes[(count + turn // 2) % len(meshes)]
            yaw = 90.0 if turn % 2 else 0.0
            hx, hy = (bounds.max.x - bounds.min.x) / 2 * BODY, (bounds.max.y - bounds.min.y) / 2 * BODY
            if yaw:
                hx, hy = hy, hx
            rect = (at.x - hx, at.y - hy, at.x + hx, at.y + hy)
            if any(touches(rect, other, GAP) for piece, other in solid if piece != box_actor):
                continue
            if any(touches(rect, other, GAP) for other in taken):
                continue
            if any(rect[0] - KEY_GAP < x < rect[2] + KEY_GAP and rect[1] - KEY_GAP < y < rect[3] + KEY_GAP for x, y in keys):
                continue
            choice = (name, mesh, yaw, rect)
            break
        if not choice:
            continue
        name, mesh, yaw, rect = choice
        house = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(at.x, at.y, ground + LIFT), unreal.Rotator(0.0, 0.0, yaw))
        if not house:
            fail("could not place " + name)
            break
        house.static_mesh_component.set_static_mesh(mesh)
        count += 1
        house.set_actor_label("House " + name.replace("_", " ") + " " + str(count))
        house.set_folder_path("Buildings")
        taken.append(rect)
        solid = [(p, r) for p, r in solid if p != box_actor]
        out("house:", name, "at", round(at.x), round(at.y), "turned", yaw, "in place of", box_actor.get_actor_label())
        actors.destroy_actor(box_actor)
    out("mesh houses placed:", count, "of", len(boxes), "house boxes")


def material():
    path = FOLDER + "/M_Ground"
    mat = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
    if mat:
        out("already there:", path)
    else:
        mat = tools.create_asset("M_Ground", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
        white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
        coords = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerCoords, -1300, 0)
        # One repeat of a texture every 8 meters.
        coords.set_editor_property("mapping_scale", 8.0)
        last = None
        for index, (name, tint) in enumerate(LAYERS):
            y = index * 420
            sample = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, y)
            sample.set_editor_property("parameter_name", name + " Texture")
            sample.set_editor_property("texture", white)
            sample.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
            MEL.connect_material_expressions(coords, "", sample, "UVs")
            color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1000, y + 240)
            color.set_editor_property("parameter_name", name + " Tint")
            color.set_editor_property("default_value", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
            mixed = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -650, y)
            MEL.connect_material_expressions(sample, "RGB", mixed, "A")
            MEL.connect_material_expressions(color, "", mixed, "B")
            weight = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerWeight, -350, y)
            weight.set_editor_property("parameter_name", name)
            weight.set_editor_property("preview_weight", 1.0 if index == 0 else 0.0)
            if not MEL.connect_material_expressions(mixed, "", weight, "Layer"):
                fail("could not connect layer " + name)
            if last:
                MEL.connect_material_expressions(last, "", weight, "Base")
            last = weight
        MEL.connect_material_property(last, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, len(LAYERS) * 420)
        rough.set_editor_property("r", 1.0)
        MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        try:
            MEL.recompile_material(mat)
        except RuntimeError as error:
            # No renderer in this run: the material is compiled when the editor or the cook next loads it.
            out("not compiled here:", str(error)[:90])
        out("made:", path)
    made.append(mat)

    path = FOLDER + "/MI_Ground"
    inst = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
    if inst:
        out("already there:", path)
    else:
        inst = tools.create_asset("MI_Ground", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(inst, mat)
        out("made:", path)
    made.append(inst)
    return inst


def landscape(layout, ground_material):
    editor = unreal.get_editor_subsystem(unreal.LandscapeEditorSubsystem)
    found = editor.get_landscapes()
    if found:
        out("already there: landscape", found[0].get_actor_label())
        land = found[0]
    else:
        # Centered on the pieces.
        lo = [1e12, 1e12]
        hi = [-1e12, -1e12]
        for piece in of_class("LevelPieceActor"):
            rect = rect_of(piece)
            lo = [min(lo[0], rect[0]), min(lo[1], rect[1])]
            hi = [max(hi[0], rect[2]), max(hi[1], rect[3])]
        # On a whole meter, so a quad's corner sits where the old floor's did.
        center = unreal.Vector(round((lo[0] + hi[0]) / 200.0) * 100.0, round((lo[1] + hi[1]) / 200.0) * 100.0, layout.get_actor_location().z + LIFT)
        size = (COMPONENTS[0] * QUADS * QUAD, COMPONENTS[1] * QUADS * QUAD)
        out("pieces from", [round(v) for v in lo], "to", [round(v) for v in hi], "landscape", size, "around", round(center.x), round(center.y))
        if size[0] < hi[0] - lo[0] + 2000 or size[1] < hi[1] - lo[1] + 2000:
            fail("the landscape is smaller than the level")
            return
        land = editor.create_landscape(center, unreal.Rotator(0.0, 0.0, 0.0), unreal.Vector(QUAD, QUAD, 100.0),
                                       unreal.IntPoint(COMPONENTS[0], COMPONENTS[1]), 1, QUADS, ground_material, 2)
        if not land:
            fail("no landscape was made")
            return
        land.set_actor_label("Ground Landscape")
        out("made: landscape", editor.get_landscape_resolution(land), "vertices, layers", [str(n) for n in editor.get_target_layer_names(land)])

    names = [str(n) for n in editor.get_target_layer_names(land)]
    for name, tint in LAYERS:
        if name not in names:
            editor.add_target_layer(land, name, None)
            out("added target layer:", name)
        info = editor.get_target_layer_info(land, name)
        if not info:
            path = FOLDER + "/" + name + "_LayerInfo"
            info = unreal.load_asset(path) if EAL.does_asset_exist(path) else editor.create_target_layer_info(name, FOLDER)
            editor.set_target_layer_info(land, name, info)
            out("layer info:", name, info.get_path_name())
        made.append(info)

    # The whole of it starts as plain ground, so it is not black before anything is painted.
    resolution = editor.get_landscape_resolution(land)
    count = resolution.x * resolution.y
    edit_layer = editor.get_edit_layer_names(land)[0]
    weights = editor.get_weightmap_data(land, edit_layer, LAYERS[0][0])
    painted = any(sum(1 for w in editor.get_weightmap_data(land, edit_layer, name) if w > 0.0) for name, tint in LAYERS[1:])
    if painted:
        out("painted already: left as it is")
    elif min(weights) < 1.0:
        editor.set_weightmap_data(land, edit_layer, LAYERS[0][0], [1.0] * count)
        out("filled:", LAYERS[0][0], "on edit layer", str(edit_layer), count, "vertices")
    land.force_layers_full_update()
    # Reading the merged result makes the engine merge the edit layers now, where a renderer is there to do it.
    merged = editor.get_weightmap_data(land, "", LAYERS[0][0])
    out("merged", LAYERS[0][0], "weights: least", round(min(merged), 3), "most", round(max(merged), 3))
    if min(merged) < 0.99:
        fail("the paint layers were not merged: run this with a renderer (see the top of this file)")
    try:
        MEL.recompile_material(ground_material.get_base_material())
    except RuntimeError as error:
        out("not compiled here:", str(error)[:90])

    for actor in of_class("StaticMeshActor"):
        if actor.get_actor_label() == "Floor":
            actors.destroy_actor(actor)
            out("removed: the template's floor")


def make():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    found = of_class("CrowdRenderer")
    if not found:
        fail("no layout actor in " + MAP + ": run Tools/Level/make_outskirts_map.py first")
        return
    layout = found[0]
    pieces(layout)
    houses(layout)
    ground_material = material()
    landscape(layout, ground_material)
    out("actors by class:", {name: len(of_class(name)) for name in ("LevelPieceActor", "StaticMeshActor", "Landscape", "CrowdRenderer")})
    out("saved level:", levels.save_current_level())


run_step("editable", make)
save_if_clean(*made)
