"""Makes the zip of a web stage that itch.io takes as an HTML game.

Usage (engine python, -I): make_web_zip.py <stage folder> <output zip> [--deploy <deploy.py>] [--id <name>] [--docs <project folder>]

It asks the engine's deploy planner which files of the stage belong in a zip upload (it leaves out the
pre-compressed copies and host files a portal does not use), then zips exactly those, with index.html at the top.
With --docs, the jam documents of the project (docs/WhatWasUsed.md, docs/GDD.md and every dev log, without their
pictures) are added in a JamDocs folder: the game does not read them.
It uploads nothing.
"""
import argparse
import json
import os
import subprocess
import sys
import tempfile
import zipfile

DEFAULT_DEPLOY = r"D:\EpicGames\UE_5.9\Engine\Platforms\Wasm\Build\Tools\deploy\deploy.py"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("stage")
    parser.add_argument("out")
    parser.add_argument("--deploy", default=DEFAULT_DEPLOY)
    parser.add_argument("--id", default="build")
    parser.add_argument("--docs", default="")
    args = parser.parse_args()

    stage = os.path.abspath(args.stage)
    with tempfile.TemporaryDirectory() as plan_dir:
        subprocess.run(
            [sys.executable, "-I", args.deploy, "--stage", stage, "--host", "zip", "--prefix", args.id, "--out", plan_dir],
            check=True, stdout=subprocess.DEVNULL)
        with open(os.path.join(plan_dir, "deploy-plan.json"), encoding="utf-8") as handle:
            plan = json.load(handle)

    if plan.get("blocked") or plan.get("problems"):
        print("FAIL: the deploy planner reported problems:", plan.get("problems"), plan.get("blocked"))
        return 1

    names = [entry["key"][len(args.id) + 1:] for entry in plan["objects"]]
    if "index.html" not in names:
        print("FAIL: the plan has no index.html at the top")
        return 1

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    if os.path.exists(args.out):
        os.remove(args.out)
    total = 0
    with zipfile.ZipFile(args.out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name in sorted(names):
            path = os.path.join(stage, name.replace("/", os.sep))
            total += os.path.getsize(path)
            archive.write(path, name)
        docs = 0
        if args.docs:
            project = os.path.abspath(args.docs)
            wanted = [("docs/WhatWasUsed.md", "JamDocs/WhatWasUsed.md"), ("docs/GDD.md", "JamDocs/GDD.md")]
            logs = os.path.join(project, "DevLog")
            wanted += [("DevLog/" + f, "JamDocs/DevLog/" + f) for f in sorted(os.listdir(logs)) if f.endswith(".md")]
            for source, name in wanted:
                path = os.path.join(project, source.replace("/", os.sep))
                if not os.path.isfile(path):
                    print("FAIL: missing " + path)
                    return 1
                total += os.path.getsize(path)
                archive.write(path, name)
                docs += 1
            print("jam documents added:", docs)
        names = names + [None] * docs

    print("OK: %d files, %.1f MB unzipped, zip %.1f MB: %s" % (
        len(names), total / 1048576.0, os.path.getsize(args.out) / 1048576.0, args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
