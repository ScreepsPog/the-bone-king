"""Headless Unreal: the game's sound channels, and the director's sound files imported onto them.

- /Game/Audio/Mix: four sound classes, SC_Master with SC_Music, SC_Effects and SC_Interface under it, and one sound
  mix, MIX_Settings. The settings screen's four volumes are set on these through the mix; a sound is on a channel
  when its Sound Class is that channel's.
- /Game/Audio/Music: every .wav under <source>/music, as MUS_<name>, on SC_Music, set to loop.
- /Game/Audio/SFX: every .wav under <source>/sound, as SFX_<name>, on SC_Effects (a name starting "ui" goes on
  SC_Interface).
Nothing is edited, trimmed or wired into the game here: the director does that in the editor.
A sound that is already there keeps its settings. Run with the editor closed: ue.py py Tools/Audio/import_sounds.py"""
from ue_bp import *  # out, fail, run_step
import os
import re
import unreal

SOURCE = os.environ.get("SOUND_SOURCE", "E:/UE5/AI Game Jam Assets/Sound")
MIX = "/Game/Audio/Mix"
CLASSES = ["SC_Master", "SC_Music", "SC_Effects", "SC_Interface"]

EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
state = {}


def classes():
    for name in CLASSES:
        path = MIX + "/" + name
        made = unreal.load_asset(path) if EAL.does_asset_exist(path) else tools.create_asset(name, MIX, unreal.SoundClass, unreal.SoundClassFactory())
        if not made:
            fail("could not make " + path)
            return
        state[name] = made
    master = state["SC_Master"]
    children = [state[name] for name in CLASSES[1:]]
    master.set_editor_property("child_classes", children)
    for child in children:
        child.set_editor_property("parent_class", master)
    path = MIX + "/MIX_Settings"
    mix = unreal.load_asset(path) if EAL.does_asset_exist(path) else tools.create_asset("MIX_Settings", MIX, unreal.SoundMix, unreal.SoundMixFactory())
    if not mix:
        fail("could not make " + path)
        return
    for item in list(state.values()) + [mix]:
        EAL.save_loaded_asset(item, False)
    out("channels:", ", ".join(CLASSES), "and", path, "| children of the master:", len(master.get_editor_property("child_classes")))


def sounds():
    if not os.path.isdir(SOURCE):
        fail("missing " + SOURCE)
        return
    made = 0
    for folder, dirs, files in os.walk(SOURCE):
        music = "music" in folder.replace("\\", "/").lower().split("/sound/", 1)[-1].split("/")[0] if "/sound/" in folder.replace("\\", "/").lower() + "/" else False
        for file in sorted(files):
            if not file.lower().endswith(".wav"):
                continue
            words = [w.capitalize() for w in re.split(r"[^A-Za-z0-9]+", file[:-4]) if w]
            name = ("MUS_" if music else "SFX_") + "_".join(words)
            dest = "/Game/Audio/Music" if music else "/Game/Audio/SFX"
            channel = "SC_Music" if music else "SC_Interface" if file.lower().startswith("ui") else "SC_Effects"
            path = dest + "/" + name
            if EAL.does_asset_exist(path):
                out("already there:", path)
                continue
            task = unreal.AssetImportTask()
            task.filename = os.path.join(folder, file)
            task.destination_path = dest
            task.destination_name = name
            task.automated = True
            task.replace_existing = False
            task.save = False
            tools.import_asset_tasks([task])
            wave = unreal.load_asset(path)
            if not wave:
                fail("did not import " + file)
                continue
            wave.set_editor_property("sound_class_object", state[channel])
            if music:
                wave.set_editor_property("looping", True)
            EAL.save_loaded_asset(wave, False)
            made += 1
            out(path, "on", channel, "%.1f s" % wave.get_editor_property("duration"))
    out("sounds imported:", made)


run_step("channels", classes)
run_step("sounds", sounds)
