"""Headless Unreal: the mannequin's unarmed attack animations step forward as they swing. The crowd's figures are moved
by the simulation, so the step is locked out: the root stays where it is. Run: ue.py py Tools/Art/lock_attack_root.py"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import unreal

made = []


def lock():
    for name in ("MM_Attack_01", "MM_Attack_02", "MM_Attack_03"):
        anim = unreal.load_asset("/Game/Characters/Mannequins/Anims/Unarmed/Attack/" + name)
        if not anim:
            fail("missing " + name)
            continue
        anim.set_editor_property("enable_root_motion", True)
        anim.set_editor_property("force_root_lock", True)
        made.append(anim)
        out(name, "root locked:", anim.get_editor_property("force_root_lock"))


run_step("lock", lock)
save_if_clean(*made)
