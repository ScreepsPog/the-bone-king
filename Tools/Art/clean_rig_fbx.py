"""Blender (background): makes a Tripo-rigged FBX safe for Unreal.

The Tripo file keeps its mesh and its armature at different scales and is about 1 unit tall. This bakes one scale into
both, so Unreal gets a model HEIGHT_CM tall whose bones and mesh agree and whose root bone has no scale.

Run: blender -b --factory-startup --python Tools/Art/clean_rig_fbx.py -- <in.fbx> <out.fbx> [height_cm=220]
"""
import sys

import bpy

args = sys.argv[sys.argv.index("--") + 1:]
src, dst = args[0], args[1]
height_cm = float(args[2]) if len(args) > 2 else 220.0

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=src)
armature = next(o for o in bpy.data.objects if o.type == "ARMATURE")
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
tall = max(m.dimensions.z for m in meshes)

# One Blender unit becomes one centimeter, so the exporter writes no scale onto the root bone.
bpy.context.scene.unit_settings.scale_length = 0.01
factor = height_cm / tall
armature.scale = (factor, factor, factor)
bpy.context.view_layer.update()
bpy.ops.object.select_all(action="DESELECT")
for o in [armature] + meshes:
    o.select_set(True)
bpy.context.view_layer.objects.active = armature
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
bpy.context.view_layer.update()

print("CLEAN height", round(max(m.dimensions.z for m in meshes), 2), "scales", [tuple(round(v, 3) for v in o.scale) for o in [armature] + meshes])
head = armature.data.bones.get("head")
if head:
    print("CLEAN head bone at", tuple(round(v, 1) for v in head.head_local))

bpy.ops.export_scene.fbx(
    filepath=dst, use_selection=True, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
    apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", global_scale=1.0, bake_anim=False,
    mesh_smooth_type="FACE", use_armature_deform_only=False, path_mode="AUTO")
print("CLEAN wrote", dst)
