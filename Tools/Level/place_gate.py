"""Headless Unreal: puts the kingdom gate into the Outskirts level as an actor (the layout actor's Place Gate As
Actor), once, and saves the level. The game finishes when the wizard walks into the gate after the commander is dead.

Needs a build that has AKingdomGateActor. Run with the editor closed: ue.py py Tools/Level/place_gate.py
Then run the unit tests: WebTesting.Level.TheOutskirtsIsPlacedInTheLevel says whether the gate stands in the open and
can be walked to from the commander. If it does not, move the Kingdom Gate actor in the editor."""
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
    if of_class("KingdomGateActor"):
        out("already there: the kingdom gate")
    else:
        found[0].place_gate_as_actor()
    gates = of_class("KingdomGateActor")
    if len(gates) != 1:
        fail("kingdom gates in the level: %d" % len(gates))
        return
    where = gates[0].get_actor_location()
    out("kingdom gate at", round(where.x), round(where.y), "yaw", round(gates[0].get_actor_rotation().yaw),
        "width", gates[0].get_editor_property("Width"), "depth", gates[0].get_editor_property("Depth"))
    out("saved level:", levels.save_current_level())


run_step("gate", make)
