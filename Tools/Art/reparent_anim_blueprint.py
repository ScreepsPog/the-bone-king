"""Headless Unreal: makes WizardAnimInstance (C++) the parent class of the mannequin's animation blueprint,
ABP_Unarmed, so a casting animation can be laid over the upper body while the blueprint's own idle, walk and run keep
the legs. The blueprint's graph is not changed.

Needs a build that has UWizardAnimInstance. Run with the editor closed: ue.py py Tools/Art/reparent_anim_blueprint.py"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import unreal

PATH = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"
made = []


def make():
    bp = unreal.EditorAssetLibrary.load_asset(PATH)
    if not bp:
        fail("missing " + PATH)
        return
    parent = unreal.WizardAnimInstance.static_class()
    generated = bp.generated_class()
    if generated and unreal.MathLibrary.class_is_child_of(generated, parent):
        out("already set:", PATH, "is a", parent.get_name())
        return
    unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    generated = bp.generated_class()
    if not (generated and unreal.MathLibrary.class_is_child_of(generated, parent)):
        fail("the parent class did not change")
        return
    out("parent of", PATH, "is now", parent.get_name())
    made.append(bp)


run_step("parent", make)
if made:
    for asset in made:
        out("saved:", unreal.EditorAssetLibrary.save_loaded_asset(asset, False))
