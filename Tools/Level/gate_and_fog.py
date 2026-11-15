"""Headless Unreal: two small changes to the Outskirts level (direction of 2026-11-15 02:28 PST), and it is saved.

- The Kingdom Gate actor (the ground the wizard walks into to finish) is put nearer the director's castle gate. It
  stays its own actor; its turn and size are kept. Skipped when it is already within GATE_NEAR of GATE_AT.
- A light fog over the whole map: the level's ExponentialHeightFog is brought up to the ground and given a low
  density, so it is barely there. Change FOG_* below and run again to change it.

Run with the editor closed: ue.py py Tools/Level/gate_and_fog.py"""
from ue_bp import *  # out, fail, run_step
import os
import unreal

MAP = "/Game/Maps/Outskirts"
GATE_AT = (18804.0, 2491.0)
GATE_NEAR = 30.0
FOG_Z = 0.0
FOG_DENSITY = float(os.environ.get("FOG_DENSITY", "0.035"))
FOG_FALLOFF = float(os.environ.get("FOG_FALLOFF", "0.05"))
FOG_COLOR = (0.035, 0.05, 0.085)
FOG_MAX_OPACITY = 0.6

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def of_class(name):
    return [a for a in actors.get_all_level_actors() if a.get_class().get_name() == name]


def change():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    gates = of_class("KingdomGateActor")
    if len(gates) != 1:
        fail("kingdom gates in the level: %d" % len(gates))
        return
    where = gates[0].get_actor_location()
    if abs(where.x - GATE_AT[0]) > GATE_NEAR or abs(where.y - GATE_AT[1]) > GATE_NEAR:
        gates[0].set_actor_location(unreal.Vector(GATE_AT[0], GATE_AT[1], where.z), False, False)
        out("kingdom gate moved from", round(where.x), round(where.y), "to", GATE_AT)
    else:
        out("kingdom gate already there:", round(where.x), round(where.y))

    fogs = of_class("ExponentialHeightFog")
    if len(fogs) != 1:
        fail("height fogs in the level: %d" % len(fogs))
        return
    spot = fogs[0].get_actor_location()
    fog = fogs[0].get_component_by_class(unreal.ExponentialHeightFogComponent)
    for name in ("fog_density", "fog_height_falloff", "fog_max_opacity", "start_distance", "fog_inscattering_luminance"):
        out("fog before:", name, fog.get_editor_property(name))
    out("fog height before:", round(spot.z))
    fogs[0].set_actor_location(unreal.Vector(spot.x, spot.y, FOG_Z), False, False)
    fog.set_editor_property("fog_density", FOG_DENSITY)
    fog.set_editor_property("fog_height_falloff", FOG_FALLOFF)
    fog.set_editor_property("fog_max_opacity", FOG_MAX_OPACITY)
    fog.set_editor_property("start_distance", 0.0)
    fog.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(FOG_COLOR[0], FOG_COLOR[1], FOG_COLOR[2], 1.0))
    out("fog now: density", FOG_DENSITY, "falloff", FOG_FALLOFF, "height", FOG_Z)
    out("saved level:", levels.save_current_level())


run_step("gate and fog", change)
