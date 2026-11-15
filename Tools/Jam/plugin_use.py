"""Headless Unreal: which engine plugins the project's content really uses, to decide which can be left out of the
web build (direction of 2026-11-15 02:55 PST: "Only remove the plugins that would for sure not have any bearing").

For every asset under /Game it gathers what the asset depends on. A dependency on /Script/<Module> is code of that
module; a dependency on a plugin's own content (/<Plugin>/...) is that plugin's content. Prints one line for each
module and each plugin content root used, with how many assets use it and an example.

Run with the editor closed: ue.py py Tools/Jam/plugin_use.py"""
from ue_bp import *  # out, fail, run_step
import unreal


def scan():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    options = unreal.AssetRegistryDependencyOptions(True, True, True, True, True)
    seen = set()
    scripts = {}
    roots = {}
    for data in registry.get_assets_by_path("/Game", recursive=True):
        package = str(data.package_name)
        if package in seen:
            continue
        seen.add(package)
        for dep in registry.get_dependencies(data.package_name, options) or []:
            dep = str(dep)
            if dep.startswith("/Script/"):
                scripts.setdefault(dep[8:], []).append(package)
            elif not dep.startswith("/Game/") and not dep.startswith("/Engine/"):
                roots.setdefault(dep.split("/")[1], []).append(package)
    out("packages under /Game:", len(seen))
    for name in sorted(scripts):
        out("SCRIPT", name, len(scripts[name]), scripts[name][0])
    for name in sorted(roots):
        out("ROOT", name, len(roots[name]), roots[name][0])


run_step("scan", scan)
