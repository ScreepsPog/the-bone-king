"""Blender (background): turns a Tripo weapon .obj (a sword) into an FBX made to be held, with its textures beside it.

The file lies any way round and is about 1 unit long. This finds the sword's own axes, stands it up with the blade
along +Z, the crossguard along X and the flat of the blade facing Y, scales it to LENGTH_CM from pommel to tip, and
puts the middle of the grip at the origin, so a hand socket holds it by the grip. Roughness and metallic, which come
as two files, are packed into one (roughness green, metallic blue), as the building material reads them.

A hammer (fifth argument "hammer") is stood with its head up: the widest place is the head, at the top, and the
origin is a third of the way up the handle, where a hand holds it.

Things that fly are held by their middle, point up (+Z), so the game can lay them along their flight: an "arrow"
has its widest place (the feathers) at the tail, a "spear" has it (the head) at the point.

Run: blender -b --factory-startup --python Tools/Art/prepare_weapon_obj.py -- <in.obj> <out_dir> <name> <length_cm> [sword|hammer|arrow|spear]
"""
import os
import shutil
import sys

import bpy
import numpy as np
from mathutils import Matrix

args = sys.argv[sys.argv.index("--") + 1:]
src, out_dir, name, length_cm = args[0], args[1], args[2], float(args[3])
kind = args[4] if len(args) > 4 else "sword"
hammer = kind == "hammer"
# A hammer's head and a spear's head are the widest place and belong at the top.
wide_top = kind in ("hammer", "spear")
out_dir = os.path.abspath(out_dir)
os.makedirs(out_dir, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.obj_import(filepath=src)
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
bpy.context.scene.unit_settings.scale_length = 0.01
bpy.ops.object.select_all(action="DESELECT")
for m in meshes:
    m.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
if len(meshes) > 1:
    bpy.ops.object.join()
sword = bpy.context.view_layer.objects.active
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

count = len(sword.data.vertices)
points = np.empty(count * 3)
sword.data.vertices.foreach_get("co", points)
points = points.reshape(count, 3)

# The sword's own axes: the longest is the blade, the next the crossguard, the least the flat of the blade.
middle = points.mean(axis=0)
values, vectors = np.linalg.eigh(np.cov((points - middle).T))
order = np.argsort(values)[::-1]
blade, guard = vectors[:, order[0]], vectors[:, order[1]]

# The crossguard is the widest place along the blade, and it sits at the hilt's end.
along = (points - middle) @ blade
across = np.abs((points - middle) @ guard)
edges = np.linspace(along.min(), along.max(), 41)
widths = [across[(along >= edges[i]) & (along <= edges[i + 1])].max(initial=0.0) for i in range(40)]
widest = int(np.argmax(widths))
# A sword's widest place (the crossguard) is at the low end; a hammer's (the head) is at the top.
if (widest > 20) != wide_top:
    blade = -blade
    along = -along
    widest = 39 - widest
    edges = np.linspace(along.min(), along.max(), 41)
flat = np.cross(blade, guard)
pommel, tip = along.min(), along.max()
grip = pommel + 0.3 * (edges[widest] - pommel) if hammer else 0.5 * (pommel + edges[widest])
if kind in ("arrow", "spear"):
    grip = 0.5 * (pommel + tip)
factor = length_cm / (tip - pommel)

turn = np.array([guard, flat, blade])  # rows: the new X, Y and Z
placed = ((points - middle) @ turn.T - np.array([0.0, 0.0, grip])) * factor
# The grip's own middle, so the hand closes around it and not beside it.
near = np.abs(placed[:, 2]) < 0.25 * (edges[widest] - pommel) * factor
if kind in ("arrow", "spear") or not near.any():
    # What flies is centered on the middle of its shaft: the points nearest its long axis. (A plain shaft may have no
    # point at all near its middle.)
    off = np.linalg.norm(placed[:, :2] - np.median(placed[:, :2], axis=0), axis=1)
    near = off <= np.percentile(off, 40)
placed[:, :2] -= 0.5 * (placed[near, :2].min(axis=0) + placed[near, :2].max(axis=0))
sword.data.vertices.foreach_set("co", placed.reshape(-1))
sword.data.update()
sword.matrix_world = Matrix.Identity(4)
size = placed.max(axis=0) - placed.min(axis=0)
print("WEAPON", name, "half widths cm from pommel to tip", [round(float(across[(along >= low) & (along <= high)].max(initial=0.0) * factor), 1) for low, high in zip(edges[:-1:4], edges[1::4])])
print("WEAPON", name, "size cm", tuple(round(float(v), 1) for v in size), "grip to tip", round(float(placed[:, 2].max()), 1),
      "grip to pommel", round(float(-placed[:, 2].min()), 1), "crossguard at", round(float((edges[widest] - grip) * factor), 1))

# Textures: color and normal as they are; roughness and metallic packed into one.
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
    print("WEAPON texture", path)


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
packed[:, :, 0] = 1.0
packed[:, :, 1] = rough
packed[:, :, 2] = metal
out = bpy.data.images.new(name + "_RM", width, height, alpha=False)
out.colorspace_settings.name = "Non-Color"
out.pixels.foreach_set(packed.reshape(-1))
out.filepath_raw = os.path.join(out_dir, name + "_RM.png")
out.file_format = "PNG"
out.save()
print("WEAPON texture", out.filepath_raw, (width, height), "roughness mean", round(float(rough.mean()), 2), "metallic mean", round(float(metal.mean()), 2))

bpy.ops.object.select_all(action="DESELECT")
sword.select_set(True)
dst = os.path.join(out_dir, name + ".fbx")
bpy.ops.export_scene.fbx(
    filepath=dst, use_selection=True, object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
    global_scale=1.0, bake_anim=False, mesh_smooth_type="FACE", path_mode="AUTO")
print("WEAPON wrote", dst)
