"""Headless Unreal: imports a rigged character whose bones are NAMED like the UE5 mannequin's onto the mannequin skeleton,
then turns every bone to the mannequin's convention so mannequin animations play on it unchanged.

Why: auto-riggers (Tripo here) name bones like the UE5 skeleton but point them along another axis. Animations store each
bone's rotation outright, so on such a rig they crumple the mesh. This keeps every joint where it is, gives each bone the
mannequin's orientation (swung to follow this model's own limbs), and leaves the mesh untouched.

Run with the editor closed, after Tools/Art/clean_rig_fbx.py:
  ue.py py Tools/Art/import_mannequin_character.py
"""
from ue_bp import *  # out, fail, run_step, save_if_clean
import math
import os
import unreal

# Which model: set the environment variable ART_MODEL (default WizardKing) before running.
#   ART_MODEL=Skeleton ue.py py Tools/Art/import_mannequin_character.py
MODELS = {
    "WizardKing": ("C:/Dev/WebTesting/SourceArt/WizardKing/Wizard_King_V2_clean.fbx", "/Game/Wizard", "SKM_Wizard_King", "/Game/Wizard/M_Wizard_King"),
    "Skeleton": ("C:/Dev/WebTesting/SourceArt/Skeleton/Skeleton_clean.fbx", "/Game/Army", "SKM_Skeleton", "/Game/Army/M_Skeleton"),
    "Guard": ("C:/Dev/WebTesting/SourceArt/Guard/Guard_clean.fbx", "/Game/Army", "SKM_Guard", "/Game/Army/M_Guard"),
    "Grunt": ("C:/Dev/WebTesting/SourceArt/Grunt/Grunt_clean.fbx", "/Game/Enemy", "SKM_Grunt", "/Game/Enemy/M_Grunt"),
    "Assassin": ("C:/Dev/WebTesting/SourceArt/Assassin/Assassin_clean.fbx", "/Game/Enemy", "SKM_Assassin", "/Game/Enemy/M_Assassin"),
    "Archer": ("C:/Dev/WebTesting/SourceArt/Archer/Archer_clean.fbx", "/Game/Enemy", "SKM_Archer", "/Game/Enemy/M_Archer"),
    "Brute": ("C:/Dev/WebTesting/SourceArt/Brute/Brute_clean.fbx", "/Game/Enemy", "SKM_Brute", "/Game/Enemy/M_Brute"),
    "Commander": ("C:/Dev/WebTesting/SourceArt/Commander/Commander_clean.fbx", "/Game/Enemy", "SKM_Commander", "/Game/Enemy/M_Commander"),
    "Marksman": ("C:/Dev/WebTesting/SourceArt/Marksman/Marksman_clean.fbx", "/Game/Army", "SKM_Marksman", "/Game/Army/M_Marksman"),
}
FBX, DEST, NAME, MATERIAL = MODELS[os.environ.get("ART_MODEL", "WizardKing")]
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
REFERENCE = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"

EAL = unreal.EditorAssetLibrary
made = []


# ---- quaternions as (x, y, z, w); global = parent * local
def q_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def q_inv(q):
    return (-q[0], -q[1], -q[2], q[3])


def q_rotate(q, v):
    r = q_mul(q_mul(q, (v[0], v[1], v[2], 0.0)), q_inv(q))
    return (r[0], r[1], r[2])


def q_norm(q):
    n = math.sqrt(sum(c * c for c in q)) or 1.0
    return tuple(c / n for c in q)


def v_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def v_unit(v):
    n = math.sqrt(sum(c * c for c in v))
    return (v[0] / n, v[1] / n, v[2] / n) if n > 1e-6 else None


def q_between(a, b):
    """Shortest turn that takes unit vector a onto unit vector b."""
    dot = sum(x * y for x, y in zip(a, b))
    if dot > 0.999999:
        return (0.0, 0.0, 0.0, 1.0)
    if dot < -0.999999:
        axis = v_unit((0.0, -a[2], a[1])) or (0.0, 0.0, 1.0)
        return (axis[0], axis[1], axis[2], 0.0)
    cross = (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
    return q_norm((cross[0], cross[1], cross[2], 1.0 + dot))


IDENTITY = (0.0, 0.0, 0.0, 1.0)


def main_children():
    """The child that says which way a bone points."""
    table = {"spine_01": "spine_02", "spine_02": "spine_03", "spine_03": "spine_04", "spine_04": "spine_05",
             "spine_05": "neck_01", "neck_01": "neck_02", "neck_02": "head"}
    for s in ("l", "r"):
        table.update({"clavicle_" + s: "upperarm_" + s, "upperarm_" + s: "lowerarm_" + s, "lowerarm_" + s: "hand_" + s,
                      "hand_" + s: "middle_metacarpal_" + s, "thigh_" + s: "calf_" + s, "calf_" + s: "foot_" + s,
                      "foot_" + s: "ball_" + s, "thumb_01_" + s: "thumb_02_" + s, "thumb_02_" + s: "thumb_03_" + s})
        for f in ("index", "middle", "ring", "pinky"):
            table.update({f + "_metacarpal_" + s: f + "_01_" + s, f + "_01_" + s: f + "_02_" + s, f + "_02_" + s: f + "_03_" + s})
    return table


def read_globals(mesh):
    m = unreal.SkeletonModifier()
    m.set_skeletal_mesh(mesh)
    data = {}
    order = []
    for name in m.get_all_bone_names():
        n = str(name)
        t = m.get_bone_transform(name, True)
        data[n] = {"pos": (t.translation.x, t.translation.y, t.translation.z),
                   "rot": q_norm((t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w)),
                   "scale": (t.scale3d.x, t.scale3d.y, t.scale3d.z),
                   "parent": str(m.get_parent_name(name))}
        order.append(n)
    return m, data, order


def import_mesh():
    path = DEST + "/" + NAME
    if EAL.does_asset_exist(path):
        fail(path + " exists: delete Content" + path[5:] + ".uasset first, this script imports it fresh")
        return
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("skeleton", unreal.load_asset(SKELETON))
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", False)
    task = unreal.AssetImportTask()
    task.filename = FBX
    task.destination_path = DEST
    task.destination_name = NAME
    task.automated = True
    task.replace_existing = True
    task.save = False
    task.options = ui
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(path)
    if not isinstance(mesh, unreal.SkeletalMesh):
        fail("import gave " + str(mesh))
        return
    made.append(mesh)
    b = mesh.get_bounds()
    out("imported", path, "height %.1f" % (b.box_extent.z * 2.0), "skeleton", mesh.get_editor_property("skeleton").get_name())


def orient():
    mesh = unreal.load_asset(DEST + "/" + NAME)
    modifier, mine, order = read_globals(mesh)
    _, manny, _ = read_globals(unreal.load_asset(REFERENCE))
    out("before: root scale", tuple(round(v, 3) for v in mine["root"]["scale"]),
        "head at", tuple(round(v, 1) for v in mine["head"]["pos"]),
        "thigh_l at", tuple(round(v, 1) for v in mine["thigh_l"]["pos"]),
        "ball_l at", tuple(round(v, 1) for v in mine["ball_l"]["pos"]))
    if mine["thigh_l"]["pos"][0] <= 0.0 or mine["ball_l"]["pos"][1] <= mine["foot_l"]["pos"][1]:
        fail("the model does not face +Y with its left side on +X, like the mannequin")
        return
    children = main_children()
    swing = {}
    new = {}
    for n in order:
        if n not in manny:
            fail("bone not on the mannequin: " + n)
            return
        parent = mine[n]["parent"]
        q = IDENTITY
        child = children.get(n)
        if child and child in mine and child in manny:
            theirs = v_unit(v_sub(manny[child]["pos"], manny[n]["pos"]))
            ours = v_unit(v_sub(mine[child]["pos"], mine[n]["pos"]))
            if theirs and ours:
                q = q_between(theirs, ours)
        elif n not in ("root", "pelvis") and parent in swing:
            q = swing[parent]  # ends and twist bones turn with their parent
        swing[n] = q
        pos = (0.0, 0.0, 0.0) if n == "root" else mine[n]["pos"]
        new[n] = {"pos": pos, "rot": q_norm(q_mul(q, manny[n]["rot"]))}
    for n in order:
        parent = mine[n]["parent"]
        if parent in new:
            inv = q_inv(new[parent]["rot"])
            rot = q_mul(inv, new[n]["rot"])
            pos = q_rotate(inv, v_sub(new[n]["pos"], new[parent]["pos"]))
        else:
            rot, pos = new[n]["rot"], new[n]["pos"]
        t = unreal.Transform()
        t.translation = unreal.Vector(pos[0], pos[1], pos[2])
        t.rotation = unreal.Quat(rot[0], rot[1], rot[2], rot[3])
        t.scale3d = unreal.Vector(1.0, 1.0, 1.0)
        if not modifier.set_bone_transform(n, t, False):
            fail("could not set " + n)
            return
    # The mannequin's helper bones. Auto-riggers leave them out, and the mannequin's foot placement rig then holds the
    # legs still. Each sits on the bone it shadows, as on the mannequin.
    extra = [("ik_foot_root", "root", None), ("ik_foot_l", "ik_foot_root", "foot_l"), ("ik_foot_r", "ik_foot_root", "foot_r"),
             ("ik_hand_root", "root", None), ("ik_hand_gun", "ik_hand_root", "hand_r"), ("ik_hand_l", "ik_hand_gun", "hand_l"),
             ("ik_hand_r", "ik_hand_gun", "hand_r"), ("center_of_mass", "root", None), ("interaction", "root", None)]
    added = []
    for name, parent, follows in extra:
        if name in mine:
            continue
        new[name] = dict(new[follows]) if follows else {"pos": (0.0, 0.0, 0.0), "rot": new["root"]["rot"]}
        inv = q_inv(new[parent]["rot"])
        rot = q_mul(inv, new[name]["rot"])
        pos = q_rotate(inv, v_sub(new[name]["pos"], new[parent]["pos"]))
        t = unreal.Transform()
        t.translation = unreal.Vector(pos[0], pos[1], pos[2])
        t.rotation = unreal.Quat(rot[0], rot[1], rot[2], rot[3])
        t.scale3d = unreal.Vector(1.0, 1.0, 1.0)
        if not modifier.add_bone(name, parent, t):
            fail("could not add " + name)
            return
        added.append(name)
    out("added helper bones:", added)
    if not modifier.commit_skeleton_to_skeletal_mesh():
        fail("commit failed")
        return
    # Read back: every joint where it was, every bone turned like the mannequin's.
    _, after, order = read_globals(mesh)
    swing.update({n: IDENTITY for n in order if n not in swing})
    worst_move = max(math.dist(after[n]["pos"], new[n]["pos"]) for n in order)
    worst_turn = max(math.degrees(2.0 * math.acos(min(1.0, abs(sum(a * b for a, b in zip(after[n]["rot"], new[n]["rot"])))))) for n in order)
    lean = max(math.degrees(2.0 * math.acos(min(1.0, abs(swing[n][3])))) for n in order)
    out("after: joints moved at most %.3f, bones off the target by at most %.3f degrees; largest swing from the mannequin's pose %.1f degrees"
        % (worst_move, worst_turn, lean))
    if worst_move > 0.05 or worst_turn > 0.05:
        fail("the skeleton did not take the new bone transforms")
    b = mesh.get_bounds()
    out("height after %.1f" % (b.box_extent.z * 2.0))


def material():
    mesh = unreal.load_asset(DEST + "/" + NAME)
    mat = unreal.load_asset(MATERIAL)
    if not mat:
        fail("no material at " + MATERIAL)
        return
    slots = list(mesh.materials)
    for slot in slots:
        slot.material_interface = mat
    mesh.set_editor_property("materials", slots)
    out("materials:", [(str(s.material_slot_name), s.material_interface.get_name()) for s in mesh.materials])


run_step("import", import_mesh)
run_step("orient", orient)
run_step("material", material)
save_if_clean(*made)
