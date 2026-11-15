"""Blender (background): turns a Tripo prop .obj (a lamp post, a barrel) into an FBX at the game's scale, standing on
the ground, with its textures beside it.

The file is about 1 unit tall. This scales it to HEIGHT_CM, puts its foot at height 0 with its middle over the origin,
and packs roughness and metallic, which come as two files, into one (roughness green, metallic blue).

Run: blender -b --factory-startup --python Tools/Art/prepare_prop_obj.py -- <in.obj> <out_dir> <name> <height_cm>
"""
import os
import shutil
import sys

import bpy
import numpy as np

args = sys.argv[sys.argv.index("--") + 1:]
src, out_dir, name, height_cm = args[0], os.path.abspath(args[1]), args[2], float(args[3])
os.makedirs(out_dir, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
# The file's up is Y; the importer stands it up on Z.
bpy.ops.wm.obj_import(filepath=src)
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
bpy.context.scene.unit_settings.scale_length = 0.01
bpy.ops.object.select_all(action="DESELECT")
for m in meshes:
    m.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
if len(meshes) > 1:
    bpy.ops.object.join()
prop = bpy.context.view_layer.objects.active
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

count = len(prop.data.vertices)
points = np.empty(count * 3)
prop.data.vertices.foreach_get("co", points)
points = points.reshape(count, 3)
lo, hi = points.min(axis=0), points.max(axis=0)
factor = height_cm / (hi[2] - lo[2])
# The foot: the middle of the lowest tenth, so a lamp's arm to one side does not pull the post off its spot.
low = points[points[:, 2] < lo[2] + 0.1 * (hi[2] - lo[2])]
foot = 0.5 * (low[:, :2].min(axis=0) + low[:, :2].max(axis=0))
placed = (points - np.array([foot[0], foot[1], lo[2]])) * factor
prop.data.vertices.foreach_set("co", placed.reshape(-1))
prop.data.update()
size = placed.max(axis=0) - placed.min(axis=0)
print("PROP", name, "size cm", tuple(round(float(v), 1) for v in size), "from", tuple(round(float(v), 1) for v in placed.min(axis=0)))

folder = os.path.dirname(src)
found = {}
for file in os.listdir(folder):
    lower = file.lower()
    for kind in ("basecolor", "normal", "roughness", "metallic"):
        if lower.endswith((".jpg", ".png")) and "_" + kind in lower:
            found[kind] = os.path.join(folder, file)
for kind, label in (("basecolor", "BaseColor"), ("normal", "Normal")):
    path = os.path.join(out_dir, name + "_" + label + os.path.splitext(found[kind])[1].lower())
    shutil.copyfile(found[kind], path)
    print("PROP texture", path)


def channel(path):
    image = bpy.data.images.load(path)
    image.colorspace_settings.name = "Non-Color"
    width, height = image.size
    pixels = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    return pixels.reshape(height, width, 4)[:, :, 0], width, height


rough, width, height = channel(found["roughness"])
metal, metal_width, metal_height = channel(found["metallic"])
assert (width, height) == (metal_width, metal_height), "roughness and metallic differ in size"
packed = np.ones((height, width, 4), dtype=np.float32)
packed[:, :, 1] = rough
packed[:, :, 2] = metal
out = bpy.data.images.new(name + "_RM", width, height, alpha=False)
out.colorspace_settings.name = "Non-Color"
out.pixels.foreach_set(packed.reshape(-1))
out.filepath_raw = os.path.join(out_dir, name + "_RM.png")
out.file_format = "PNG"
out.save()
print("PROP texture", out.filepath_raw, (width, height))

bpy.ops.object.select_all(action="DESELECT")
prop.select_set(True)
dst = os.path.join(out_dir, name + ".fbx")
bpy.ops.export_scene.fbx(
    filepath=dst, use_selection=True, object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
    global_scale=1.0, bake_anim=False, mesh_smooth_type="FACE", path_mode="AUTO")
print("PROP wrote", dst)
