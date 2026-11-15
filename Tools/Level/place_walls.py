"""Headless Unreal: puts every wall of the Outskirts into the level as a Wall actor (the layout actor's Place Walls As
Actors), once, so the walls can be seen, moved and sized in the editor; builds every placed piece again (the river's
pieces no longer draw their own flat water); and saves the level. Nothing else in the level is touched.

Needs a build that has AWallActor. Run with the editor closed: ue.py py Tools/Level/place_walls.py"""
from ue_bp import *  # out, fail, run_step
import unreal

MAP = "/Game/Maps/Outskirts"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def of_class(name):
    return [a for a in actors.get_all_level_actors() if a.get_class().get_name() == name]


def make():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    found = of_class("CrowdRenderer")
    if not found:
        fail("no layout actor in " + MAP)
        return
    before = len(of_class("WallActor"))
    if before:
        out("already there:", before, "walls")
    else:
        found[0].place_walls_as_actors()
    found[0].refresh_placed_pieces()
    walls = of_class("WallActor")
    if not walls:
        fail("no walls were placed")
        return
    sizes = sorted((round(w.get_editor_property("Size").x), round(w.get_editor_property("Size").y)) for w in walls)
    out("walls in the level:", len(walls), "smallest", sizes[0], "largest", sizes[-1])
    out("saved level:", levels.save_current_level())


run_step("walls", make)
