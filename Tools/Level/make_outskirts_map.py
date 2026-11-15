"""Headless Unreal: makes /Game/Maps/Outskirts, the level the game is played in, so it can be opened and edited.

It is a copy of the engine's default template (floor, sky, light, player start) with one "Outskirts Layout" actor in it.
That actor draws the river, roads, fields, trees and house boxes in the editor exactly as the game lays them out, so
buildings can be placed against them. Makes only what is missing. Run: ue.py py Tools/Level/make_outskirts_map.py"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import unreal

MAP = "/Game/Maps/Outskirts"
TEMPLATE = "/Engine/Maps/Templates/Template_Default"
LABEL = "Outskirts Layout"


def make():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        out("already there:", MAP)
        if not levels.load_level(MAP):
            fail("could not open " + MAP)
            return
    elif not levels.new_level_from_template(MAP, TEMPLATE):
        fail("could not make " + MAP)
        return

    floor_top = 0.0
    layout = None
    for actor in actors.get_all_level_actors():
        if actor.get_class().get_name() == "CrowdRenderer":
            layout = actor
        elif isinstance(actor, unreal.StaticMeshActor) and "floor" in actor.get_actor_label().lower():
            origin, extent = actor.get_actor_bounds(False)
            floor_top = origin.z + extent.z
            out("floor:", actor.get_actor_label(), "top at", round(floor_top, 1))
    if layout:
        out("already there:", LABEL)
    else:
        cls = unreal.load_class(None, "/Script/WebTesting.CrowdRenderer")
        layout = actors.spawn_actor_from_class(cls, unreal.Vector(0.0, 0.0, floor_top))
        if not layout:
            fail("could not place the layout actor")
            return
        layout.set_actor_label(LABEL)
        out("placed:", LABEL, "at height", round(floor_top, 1))
    out("actors:", sorted(a.get_actor_label() for a in actors.get_all_level_actors()))
    out("saved:", levels.save_current_level())


run_step("map", make)
