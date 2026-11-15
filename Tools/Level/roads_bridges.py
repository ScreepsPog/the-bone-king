"""Headless Unreal: three changes to the Outskirts level the director asked for, and nothing else.

- Every road block (the "Path" pieces) is deleted: the roads are what is painted on the landscape.
- Each bridge block gets the bridge model over it ("Bridge Model N", folder "River"), turned to span the river and
  pressed down in height so its deck lies just over the ground where the wizard and the crowd walk, and over the
  water. The block stays as the way across and no longer draws itself.
- (It used to give every building one tight box. The director has since made each building's collision by hand,
  so it no longer does.)

A bridge model that is already in the level is left exactly as it is.
With the editor closed: ue.py py Tools/Level/roads_bridges.py"""
from ue_bp import *  # out, fail, run_step
import unreal

MAP = "/Game/Maps/Outskirts"
BRIDGE = "/Game/Props/SM_Bridge"
# The model is 1100 long, 812 wide and 434 tall as imported. Its height is pressed to this share, and it is sunk this
# far, so the deck is near the ground the crowd walks on.
BRIDGE_HEIGHT_SHARE = 0.32
BRIDGE_SINK = 0.0
# The model's long side lies along its X; the river runs along the level's X, so the bridge is turned to cross it.
BRIDGE_YAW = 90.0

EAL = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def of_class(name):
    return [a for a in actors.get_all_level_actors() if a.get_class().get_name() == name]


def kind_of(piece):
    return str(piece.get_editor_property("Kind")).upper()


def make():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    layout = of_class("CrowdRenderer")
    if not layout:
        fail("no layout actor in " + MAP)
        return

    paths = [p for p in of_class("LevelPieceActor") if "PATH" in kind_of(p)]
    for piece in paths:
        actors.destroy_actor(piece)
    out("road blocks deleted:", len(paths), "pieces left:", len(of_class("LevelPieceActor")))

    mesh = unreal.load_asset(BRIDGE)
    if not mesh:
        fail("missing " + BRIDGE)
        return
    box = mesh.get_bounding_box()
    out("bridge model size", [round(box.max.x - box.min.x), round(box.max.y - box.min.y), round(box.max.z - box.min.z)], "lowest", round(box.min.z))
    bridges = sorted((p for p in of_class("LevelPieceActor") if "BRIDGE" in kind_of(p)), key=lambda p: p.get_actor_label())
    placed = {a.get_actor_label(): a for a in of_class("StaticMeshActor") if a.get_actor_label().startswith("Bridge Model")}
    for index, piece in enumerate(bridges):
        label = "Bridge Model %d" % (index + 1)
        where = piece.get_actor_location()
        spot = unreal.Vector(where.x, where.y, where.z - BRIDGE_SINK)
        actor = placed.get(label)
        if actor:
            # The director has placed, sized and given collision to the bridges since: one that is there is left as it is.
            out(label, "is there: left as it is")
            continue
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, spot)
        actor.set_actor_label(label)
        actor.set_folder_path("River")
        component = actor.static_mesh_component
        component.set_static_mesh(mesh)
        component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
        # The piece under it is the way across; the model is only seen.
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_actor_location_and_rotation(spot, unreal.Rotator(0.0, 0.0, piece.get_actor_rotation().yaw + BRIDGE_YAW), False, False)
        actor.set_actor_scale3d(unreal.Vector(1.0, 1.0, BRIDGE_HEIGHT_SHARE))
        out(label, "at", round(spot.x), round(spot.y), round(spot.z, 1), "top at", round(spot.z + box.max.z * BRIDGE_HEIGHT_SHARE))

    # The buildings' collision is the director's now (made in the mesh editor): it is not replaced here any more.
    layout[0].refresh_placed_pieces()
    out("saved level:", levels.save_current_level())


run_step("level", make)
