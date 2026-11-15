"""Gives the bridge model (/Game/Props/SM_Bridge) collision made for walking: flat ramps along its deck and a thin
rail down each side, read from the model itself. Run again after the model changes.

Run (editor closed): ue.py py Tools/Level/bridge_collision.py
"""
import unreal

count = unreal.CrowdRenderer.rebuild_bridge_collision()
print("BRIDGE boxes", count)
if count > 0:
    unreal.EditorAssetLibrary.save_asset("/Game/Props/SM_Bridge", only_if_is_dirty=False)
    body = unreal.load_asset("/Game/Props/SM_Bridge").get_editor_property("body_setup")
    for box in body.get_editor_property("agg_geom").get_editor_property("box_elems"):
        c = box.get_editor_property("center")
        r = box.get_editor_property("rotation")
        print("BRIDGE box", str(box.get_editor_property("name")), round(c.x), round(c.y), round(c.z), "pitch", round(r.pitch, 1),
              "size", round(box.get_editor_property("x")), round(box.get_editor_property("y")), round(box.get_editor_property("z")))
