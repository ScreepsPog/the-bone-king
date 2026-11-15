"""Pass/fail checks for jam packages. Stdlib only. Exit code 0 = pass, 1 = fail.

  jam_checks.py renderer <DefaultEngine.ini>
  jam_checks.py size <folder-or-zip> --max-total-mb N --max-file-mb N
  jam_checks.py zip <file.zip> --require-top index.html
  jam_checks.py --self-test
"""
import argparse
import os
import sys
import unittest
import zipfile

RENDERER_SECTION = "/Script/Engine.RendererSettings"
REQUIRED = {
    "r.DynamicGlobalIlluminationMethod": "0",
    "r.ReflectionMethod": "0",
    "r.Shadow.Virtual.Enable": "0",
    "r.RayTracing": "False",
    "r.Substrate": "False",
    "r.Mobile.Forward.EnableClusteredReflections": "True",
}
MB = 1024 * 1024


def check_renderer(ini_text):
    """Failures for renderer keys that are missing or wrong in the renderer section."""
    found = {}
    section = None
    for raw in ini_text.splitlines():
        line = raw.strip()
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1]
        elif section == RENDERER_SECTION and "=" in line and not line.startswith(";"):
            key, value = line.split("=", 1)
            found[key.strip().lstrip("+")] = value.strip()
    failures = []
    for key, want in REQUIRED.items():
        got = found.get(key)
        if got is None:
            failures.append(f"{key}: missing, want {want}")
        elif got.lower() != want.lower():
            failures.append(f"{key}: is {got}, want {want}")
    return failures


def check_size(sizes, max_total_mb, max_file_mb):
    """Failures for files at or over the per-file limit and a total at or over the total limit."""
    if not sizes:
        return ["no files found: the package is empty"]
    failures = []
    for name, size in sorted(sizes.items()):
        if size >= max_file_mb * MB:
            failures.append(f"{name}: {size / MB:.1f} MB, limit {max_file_mb} MB per file")
    total = sum(sizes.values())
    if total >= max_total_mb * MB:
        failures.append(f"total: {total / MB:.1f} MB, limit {max_total_mb} MB")
    return failures


def check_zip_top(names, required):
    """Failure when the required file is not at the top level of the zip."""
    if required in names:
        return []
    nested = [n for n in names if n.replace("\\", "/").endswith("/" + required)]
    where = f" (found at {nested[0]})" if nested else ""
    return [f"{required}: not at the top level of the zip{where}"]


def read_sizes(path):
    """File sizes by relative name, from a folder or from a zip's uncompressed entries."""
    if os.path.isdir(path):
        sizes = {}
        for root, _dirs, files in os.walk(path):
            for name in files:
                full = os.path.join(root, name)
                sizes[os.path.relpath(full, path).replace("\\", "/")] = os.path.getsize(full)
        return sizes
    with zipfile.ZipFile(path) as z:
        return {i.filename: i.file_size for i in z.infolist() if not i.is_dir()}


def main(argv):
    parser = argparse.ArgumentParser(description="Pass/fail checks for jam packages.")
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("renderer", help="renderer settings in DefaultEngine.ini")
    p.add_argument("ini")
    p = sub.add_parser("size", help="size limits of a folder or zip")
    p.add_argument("path")
    p.add_argument("--max-total-mb", type=int, required=True)
    p.add_argument("--max-file-mb", type=int, required=True)
    p = sub.add_parser("zip", help="a file must sit at the top level of the zip")
    p.add_argument("path")
    p.add_argument("--require-top", required=True)
    args = parser.parse_args(argv)

    if args.command == "renderer":
        with open(args.ini, encoding="utf-8-sig") as f:
            failures = check_renderer(f.read())
        summary = f"renderer settings in {args.ini}"
    elif args.command == "size":
        if not os.path.exists(args.path):
            print(f"FAIL {args.path}: not found")
            return 1
        sizes = read_sizes(args.path)
        failures = check_size(sizes, args.max_total_mb, args.max_file_mb)
        biggest = max(sizes.items(), key=lambda kv: kv[1], default=("none", 0))
        summary = (f"{len(sizes)} files, total {sum(sizes.values()) / MB:.1f} MB, "
                   f"largest {biggest[0]} {biggest[1] / MB:.1f} MB")
    else:
        with zipfile.ZipFile(args.path) as z:
            failures = check_zip_top(z.namelist(), args.require_top)
        summary = f"{args.require_top} at the top of {args.path}"

    for failure in failures:
        print("FAIL " + failure)
    print(("FAILED: " if failures else "PASS: ") + summary)
    return 1 if failures else 0


class T(unittest.TestCase):
    def test_renderer_flags_each_wrong_or_missing_key(self):
        bad = "[/Script/Engine.RendererSettings]\nr.RayTracing=True\n"
        self.assertEqual(len(check_renderer(bad)), 6)

    def test_renderer_passes_when_all_set(self):
        good = "[/Script/Engine.RendererSettings]\n" + "\n".join(f"{k}={v}" for k, v in REQUIRED.items())
        self.assertEqual(check_renderer(good), [])

    def test_renderer_ignores_keys_in_other_sections(self):
        other = "[/Script/Other]\n" + "\n".join(f"{k}={v}" for k, v in REQUIRED.items())
        self.assertEqual(len(check_renderer(other)), 6)

    def test_limits_are_strict(self):
        self.assertEqual(len(check_size({"a": 200 * MB}, 500, 200)), 1)
        self.assertEqual(len(check_size({"a": 250 * MB - 1, "b": 250 * MB + 1}, 500, 300)), 1)
        self.assertEqual(check_size({"a": 200 * MB - 1}, 500, 200), [])

    def test_empty_stage_fails(self):
        self.assertEqual(len(check_size({}, 500, 200)), 1)

    def test_zip_requires_top_level_index(self):
        self.assertEqual(len(check_zip_top(["Wasm/index.html"], "index.html")), 1)
        self.assertEqual(check_zip_top(["index.html", "a/b.wasm"], "index.html"), [])


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        sys.argv.remove("--self-test")
        unittest.main()
    else:
        sys.exit(main(sys.argv[1:]))
