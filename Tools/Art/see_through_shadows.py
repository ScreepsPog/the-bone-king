"""Stops the see-through circle from cutting shadows.

The circle around the cursor is cut out of walls, roofs and leaves by their opacity mask, measured on the screen.
A shadow is drawn from the light's point of view, where "the screen" is the light's, so the same circle cut holes
in shadows at places that had nothing to do with the cursor: shadows came and went as the mouse moved. Each of
those materials now gives the shadow pass its mask without the circle (through a Shadow Pass Switch).

Run once (editor closed): ue.py py Tools/Art/see_through_shadows.py
"""
import unreal

MATERIALS = [
    "/Game/Level/M_SeeThrough",
    "/Game/Buildings/M_Barn",
    "/Game/Buildings/M_Burned_Cottage",
    "/Game/Buildings/M_Peasant_Cottage",
    "/Game/Buildings/M_Twostory_House",
    "/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Master_Materials/M_Environment_Basic",
    "/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Master_Materials/M_Foliage_Leaves_Master_STARTER",
]
MARK = "SeeThroughShadow"
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

for path in MATERIALS:
    mat = unreal.load_asset(path)
    if not mat:
        print("SHADOW missing", path)
        continue
    mask = MEL.get_material_property_input_node(mat, unreal.MaterialProperty.MP_OPACITY_MASK)
    if not mask:
        print("SHADOW no mask on", path)
        continue
    if isinstance(mask, unreal.MaterialExpressionShadowReplace):
        print("SHADOW already done", path)
        continue
    output = MEL.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_OPACITY_MASK)
    # What the material cut before the circle was added (leaves): the first input of the multiply that joins the two.
    before = None
    before_output = ""
    if isinstance(mask, unreal.MaterialExpressionMultiply):
        inputs = MEL.get_inputs_for_material_expression(mat, mask)
        if inputs and inputs[0] and not isinstance(inputs[0], unreal.MaterialExpressionCustom):
            before = inputs[0]
            before_output = MEL.get_input_node_output_name_for_material_expression(mask, before)
    switch = MEL.create_material_expression(mat, unreal.MaterialExpressionShadowReplace, 5600, 3300)
    ok = MEL.connect_material_expressions(mask, output, switch, "Default")
    if before:
        ok = MEL.connect_material_expressions(before, before_output, switch, "Shadow") and ok
    else:
        one = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, 5400, 3500)
        one.set_editor_property("r", 1.0)
        ok = MEL.connect_material_expressions(one, "", switch, "Shadow") and ok
    ok = MEL.connect_material_property(switch, "", unreal.MaterialProperty.MP_OPACITY_MASK) and ok
    try:
        MEL.recompile_material(mat)
    except RuntimeError as error:
        pass
    EAL.set_metadata_tag(mat, MARK, "1")
    print("SHADOW", "fixed" if ok else "FAILED", path, "kept its own mask for shadows:" if before else "shadows are whole:", before.get_name() if before else "",
          "saved:", EAL.save_loaded_asset(mat, False))
print("SHADOW done")
