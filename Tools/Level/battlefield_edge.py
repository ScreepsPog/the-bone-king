"""Headless Unreal: the Outskirts loses its walls and gets a battlefield with a soft edge, and the ground past the
old walls is dressed so the edge does not look out on nothing.

- The four Wall actors are deleted. One Battlefield actor ("Battlefield", folder "Systems") is put where they
  stood: the box inside them. Size and move it in the editor. Outside it the game warns the wizard to return.
- Past the west, south and north edges (not the east: the kingdom's wall and gate go there), houses and lamp posts
  are placed (folder "Edge Dressing"): copies of the level's own, turned at random, clear of what already stands.
- The river is carried on five pieces past the west edge. Run Tools/Level/make_river_water.py after this to draw it.
- Nothing that was in the level is moved. The landscape's paint is not touched.

Done once: with a Battlefield actor in the level, it only reports. Needs a build that has ABattlefieldActor.
Run with the editor closed: ue.py py Tools/Level/battlefield_edge.py"""
from ue_bp import *  # out, fail, run_step
import math
import random
import unreal

MAP = "/Game/Maps/Outskirts"
SEED = 11
# How far past the old walls the dressing reaches, and how many of each go on each side.
REACH = 2100.0
HOUSES = {"west": 5, "south": 8, "north": 8}
LAMPS = {"west": 4, "south": 6, "north": 6}
HOUSE_CLEAR = 1150.0
LAMP_CLEAR = 450.0
RIVER_HALF = 800.0
RIVER_PIECES = 5

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def of_class(name):
    return [a for a in actors.get_all_level_actors() if a.get_class().get_name() == name]


def make():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    if of_class("BattlefieldActor"):
        out("already there: the battlefield;", len([a for a in of_class("StaticMeshActor") if a.get_folder_path() == "Edge Dressing"]), "pieces of edge dressing")
        return
    walls = of_class("WallActor")
    if len(walls) != 4:
        fail("expected the four walls, found %d" % len(walls))
        return
    # The box inside the walls: from each wall's inner face.
    xs = sorted(w.get_actor_location().x for w in walls if w.get_editor_property("Size").x < w.get_editor_property("Size").y)
    ys = sorted(w.get_actor_location().y for w in walls if w.get_editor_property("Size").x > w.get_editor_property("Size").y)
    thick = 100.0
    low = (xs[0] + thick, ys[0] + thick)
    high = (xs[-1] - thick, ys[-1] - thick)
    z = walls[0].get_actor_location().z
    for wall in walls:
        actors.destroy_actor(wall)
    field = actors.spawn_actor_from_class(unreal.BattlefieldActor, unreal.Vector((low[0] + high[0]) * 0.5, (low[1] + high[1]) * 0.5, z))
    field.set_actor_label("Battlefield")
    field.set_folder_path("Systems")
    field.set_editor_property("Size", unreal.Vector2D(high[0] - low[0], high[1] - low[1]))
    out("battlefield from", [round(v) for v in low], "to", [round(v) for v in high], "walls deleted: 4")

    # What already stands, to keep clear of; and one of each house and lamp to copy.
    taken = []
    houses = {}
    lamp = None
    river_y = None
    river_west = None
    for actor in actors.get_all_level_actors():
        name = actor.get_class().get_name()
        where = actor.get_actor_location()
        if name == "StaticMeshActor" and actor.static_mesh_component.static_mesh:
            path = actor.static_mesh_component.static_mesh.get_path_name()
            taken.append((where.x, where.y))
            if path.startswith("/Game/Buildings/"):
                houses.setdefault(path, actor)
            elif path.startswith("/Game/Props/SM_Lamp_Post"):
                lamp = lamp or actor
        elif name == "LevelPieceActor" and "RIVER" in str(actor.get_editor_property("Kind")).upper():
            if river_west is None or where.x < river_west.get_actor_location().x:
                river_west = actor
    if not houses or not lamp or not river_west:
        fail("the level has no house, lamp post or river to copy")
        return
    river_y = river_west.get_actor_location().y

    # The river, carried on past the west edge.
    size = river_west.get_editor_property("Size")
    for index in range(1, RIVER_PIECES + 1):
        spot = river_west.get_actor_location()
        piece = actors.spawn_actor_from_class(unreal.LevelPieceActor, unreal.Vector(spot.x - size.x * index, spot.y, spot.z))
        piece.set_editor_property("Kind", river_west.get_editor_property("Kind"))
        piece.set_editor_property("Size", size)
        piece.set_actor_label("Edge River %d" % index)
        piece.set_folder_path("Edge Dressing")
    out("river carried west by", RIVER_PIECES, "pieces, to x", round(river_west.get_actor_location().x - size.x * RIVER_PIECES))

    rng = random.Random(SEED)
    zones = {
        "west": ((low[0] - REACH, low[1] + 300.0), (low[0] - 350.0, high[1] - 300.0)),
        "south": ((low[0] - REACH * 0.6, low[1] - REACH), (high[0] - 900.0, low[1] - 350.0)),
        "north": ((low[0] - REACH * 0.6, high[1] + 350.0), (high[0] - 900.0, high[1] + REACH)),
    }

    def free(x, y, clear, side):
        if side == "west" and abs(y - river_y) < RIVER_HALF + clear * 0.5:
            return False
        return all(math.hypot(x - tx, y - ty) >= clear for tx, ty in taken)

    def place(source, x, y, yaw, label):
        made = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, source.get_actor_location().z))
        component = made.static_mesh_component
        component.set_static_mesh(source.static_mesh_component.static_mesh)
        component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
        made.set_actor_rotation(unreal.Rotator(0.0, 0.0, yaw), False)
        made.set_actor_scale3d(source.get_actor_scale3d())
        made.set_actor_label(label)
        made.set_folder_path("Edge Dressing")
        taken.append((x, y))

    kinds = sorted(houses)
    counts = {"houses": 0, "lamps": 0}
    for side, (zone_low, zone_high) in zones.items():
        for want, clear, kind in ((HOUSES[side], HOUSE_CLEAR, "houses"), (LAMPS[side], LAMP_CLEAR, "lamps")):
            placed = 0
            for attempt in range(want * 60):
                if placed >= want:
                    break
                x = rng.uniform(zone_low[0], zone_high[0])
                y = rng.uniform(zone_low[1], zone_high[1])
                if not free(x, y, clear, side):
                    continue
                counts[kind] += 1
                placed += 1
                if kind == "houses":
                    place(houses[rng.choice(kinds)], x, y, rng.choice([0.0, 90.0, 180.0, 270.0]) + rng.uniform(-25.0, 25.0), "Edge House %d" % counts[kind])
                else:
                    place(lamp, x, y, rng.uniform(0.0, 360.0), "Edge Lamp %d" % counts[kind])
            if placed < want:
                out("only", placed, "of", want, kind, "fitted on the", side)
    out("edge dressing: houses", counts["houses"], "lamp posts", counts["lamps"])
    layout = of_class("CrowdRenderer")
    if layout:
        layout[0].refresh_placed_pieces()
    out("saved level:", levels.save_current_level())


run_step("edge", make)
