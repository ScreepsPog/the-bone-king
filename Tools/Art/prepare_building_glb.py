"""Blender (background): turns a Tripo building .glb into an FBX at the game's scale, with its textures beside it.

The .glb is about 1 unit across and stands on a slab. This scales it so its doorway is DOOR_CM tall (the wizard is 220),
puts the doorstep at height 0 (the slab goes under the ground), and writes centimeters.

Run: blender -b --factory-startup --python Tools/Art/prepare_building_glb.py -- <in.glb> <out_dir> <name> <door_units> <floor_units> [door_cm=230]
  door_units:  the doorway's height in the file's own units (measured on a straight-on render)
  floor_units: the height of the doorstep above the bottom of the file, in the file's own units
"""
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
src, out_dir, name = args[0], args[1], args[2]
door_units, floor_units = float(args[3]), float(args[4])
door_cm = float(args[5]) if len(args) > 5 else 230.0
os.makedirs(out_dir, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
meshes = [o for o in bpy.data.objects if o.type == "MESH"]

# One Blender unit becomes one centimeter.
bpy.context.scene.unit_settings.scale_length = 0.01
factor = door_cm / door_units
bpy.ops.object.select_all(action="DESELECT")
for m in meshes:
    # Out of any parent, keeping where it is, so one scale and one move do the whole job.
    world = m.matrix_world.copy()
    m.parent = None
    m.matrix_world = world
    m.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.context.view_layer.update()
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for m in meshes:
    m.scale = (factor, factor, factor)
    m.location = (0.0, 0.0, -floor_units * factor)
bpy.context.view_layer.update()
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

lo = Vector((1e9, 1e9, 1e9))
hi = Vector((-1e9, -1e9, -1e9))
for m in meshes:
    for c in m.bound_box:
        w = m.matrix_world @ Vector(c)
        lo = Vector(map(min, lo, w))
        hi = Vector(map(max, hi, w))
print("BUILDING", name, "factor", round(factor, 1), "size cm", tuple(round(v) for v in (hi - lo)), "from z", round(lo.z), "to", round(hi.z))

# The textures: base color, normal, and roughness (green) with metallic (blue) in one.
for img in bpy.data.images:
    lower = img.name.lower()
    kind = "BaseColor" if "basecolor" in lower else "Normal" if "normal" in lower else "RM" if "_rm" in lower else None
    if not kind or not img.packed_file:
        continue
    # The file's own bytes, as they were packed into the .glb: nothing is compressed a second time.
    ending = ".png" if lower.endswith(".png") else ".jpg"
    path = os.path.join(out_dir, name + "_" + kind + ending)
    with open(path, "wb") as handle:
        handle.write(img.packed_file.data)
    print("BUILDING texture", path, tuple(img.size))

dst = os.path.join(out_dir, name + ".fbx")
bpy.ops.export_scene.fbx(
    filepath=dst, use_selection=True, object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
    global_scale=1.0, bake_anim=False, mesh_smooth_type="FACE", path_mode="AUTO")
print("BUILDING wrote", dst)
