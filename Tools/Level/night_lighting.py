"""Headless Unreal: lights the Outskirts as night that can still be played in. Run again after changing a number
here; it changes the level's own light actors (and adds one post process volume, "Night Look"), then saves the level.

- The sun becomes a moon: dim, cold blue, still casting the shadows.
- The sky is darkened to navy, and the sky light is raised and tinted so the shadows never go black.
- Exposure is fixed (no eye adaptation), so the night stays dark instead of being brightened back to day, and it
  reads the same on every renderer.
- Fog is dark blue; the picture is a little less colorful, with darker corners.

Run with the editor closed: ue.py py Tools/Level/night_lighting.py
The unit test WebTesting.Level.TheOutskirtsIsLitAsNight checks the result."""
from ue_bp import *  # out, fail, run_step
import unreal

MAP = "/Game/Maps/Outskirts"
MOON_LUX = 3.0
MOON_COLOR = (170, 195, 255)
SKY_LUMINANCE = (0.22, 0.28, 0.5)
SKY_LIGHT = 3.5
SKY_LIGHT_COLOR = (165, 190, 255)
FOG_COLOR = (0.012, 0.02, 0.045)
EXPOSURE_BIAS = 1.2
SATURATION = 0.85
VIGNETTE = 0.55
VOLUME_LABEL = "Night Look"

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def of_class(name):
    return [a for a in actors.get_all_level_actors() if a.get_class().get_name() == name]


def color(rgb):
    return unreal.Color(r=rgb[0], g=rgb[1], b=rgb[2], a=255)


def make():
    if not levels.load_level(MAP):
        fail("could not open " + MAP)
        return
    suns = of_class("DirectionalLight")
    skies = of_class("SkyLight")
    airs = of_class("SkyAtmosphere")
    fogs = of_class("ExponentialHeightFog")
    if len(suns) != 1 or len(skies) != 1 or len(airs) != 1:
        fail("expected one sun, one sky light and one atmosphere: %d %d %d" % (len(suns), len(skies), len(airs)))
        return

    moon = suns[0].get_component_by_class(unreal.DirectionalLightComponent)
    moon.set_editor_property("intensity", MOON_LUX)
    moon.set_editor_property("use_temperature", False)
    moon.set_editor_property("light_color", color(MOON_COLOR))
    out("moon", moon.get_editor_property("intensity"), "lux", MOON_COLOR)

    air = airs[0].get_component_by_class(unreal.SkyAtmosphereComponent)
    air.set_editor_property("sky_luminance_factor", unreal.LinearColor(SKY_LUMINANCE[0], SKY_LUMINANCE[1], SKY_LUMINANCE[2], 1.0))

    sky = skies[0].get_component_by_class(unreal.SkyLightComponent)
    sky.set_editor_property("intensity", SKY_LIGHT)
    sky.set_editor_property("light_color", color(SKY_LIGHT_COLOR))
    out("sky light", sky.get_editor_property("intensity"))

    for fog_actor in fogs:
        fog = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
        fog.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(FOG_COLOR[0], FOG_COLOR[1], FOG_COLOR[2], 1.0))
        out("fog density", fog.get_editor_property("fog_density"), "falloff", fog.get_editor_property("fog_height_falloff"))

    volumes = [a for a in of_class("PostProcessVolume") if a.get_actor_label() == VOLUME_LABEL]
    if volumes:
        volume = volumes[0]
    else:
        volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0.0, 0.0, 0.0))
        volume.set_actor_label(VOLUME_LABEL)
    volume.set_editor_property("unbound", True)
    look = volume.get_editor_property("settings")
    look.set_editor_property("override_auto_exposure_method", True)
    look.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    look.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
    look.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
    look.set_editor_property("override_auto_exposure_bias", True)
    look.set_editor_property("auto_exposure_bias", EXPOSURE_BIAS)
    look.set_editor_property("override_color_saturation", True)
    look.set_editor_property("color_saturation", unreal.Vector4(SATURATION, SATURATION, SATURATION, 1.0))
    look.set_editor_property("override_vignette_intensity", True)
    look.set_editor_property("vignette_intensity", VIGNETTE)
    volume.set_editor_property("settings", look)
    out("night look volume: exposure fixed, bias", EXPOSURE_BIAS, "saturation", SATURATION, "vignette", VIGNETTE)
    out("saved level:", levels.save_current_level())


run_step("night", make)
