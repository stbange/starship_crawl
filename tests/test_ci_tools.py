"""Regression checks for exhaustive CI groups and retained failure diagnostics."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("ci_runner", ROOT / "scripts/run-ci-tests.py")
ci = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ci)


class PartitionTests(unittest.TestCase):
    def test_browser_inventory_is_exhaustive_and_disjoint(self):
        config = json.loads(ci.CONFIG.read_text())
        inventory = {p.name for p in (ROOT / "tests").glob("test_*browser.py")}
        groups = ci.browser_groups(config, inventory)
        self.assertEqual(sum(map(len, groups.values())), len(inventory) - 1)
        with self.assertRaisesRegex(ValueError, "missing"):
            ci.browser_groups(config, inventory | {"test_new_browser.py"})
        config["browser"]["arrival"].append("test_browser.py")
        with self.assertRaisesRegex(ValueError, "duplicates"):
            ci.browser_groups(config, inventory)

    def test_stale_and_empty_groups_fail(self):
        for groups in ({"a": []}, {"a": ["obsolete"]}, {"a": ["one"], "b": ["one"]}, {"bad'name": ["one"]}):
            with self.assertRaises(ValueError):
                ci.validate_partition(groups, {"one"})

    def test_matrix_comes_from_the_validated_plan(self):
        result = subprocess.check_output([sys.executable, str(ROOT / "scripts/run-ci-tests.py"),
                                          "browser", "--matrix"], text=True)
        key, value = result.strip().split("=", 1)
        self.assertEqual(key, "suites")
        self.assertEqual(json.loads(value), list(json.loads(ci.CONFIG.read_text())["browser"]))

    def test_ctest_new_cases_remain_required_and_units_stay_out_of_gameplay(self):
        config = {"instrumented": {ci.REMAINDER: [], "long": ["journey"]}}
        tests = [{"name": name, "properties": [{"name": "LABELS", "value": [label]}]}
                 for name, label in [("journey", "gameplay"), ("new-journey", "gameplay"), ("unit", "unit")]]
        self.assertEqual(ci.ctest_groups(config, tests)[ci.REMAINDER], ["new-journey", "unit"])
        self.assertEqual(ci.ctest_groups(config, tests, "gameplay")[ci.REMAINDER], ["new-journey"])
        with self.assertRaises(ValueError):
            ci.ctest_groups(config, tests[1:])
        with self.assertRaises(ValueError):
            ci.ctest_groups(config, tests + tests[:1])

    def test_browser_runner_retains_exit_status_log_and_timing(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            probe = path / "probe.py"
            probe.write_text("import sys\nprint('progress',flush=True)\nprint('specific failure',file=sys.stderr)\nsys.exit(7)\n")
            env = {**os.environ, "GITHUB_ACTIONS": "true"}
            result = subprocess.run([sys.executable, str(ROOT / "scripts/run-browser-test.py"), str(probe)],
                                    cwd=path, env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 7)
            self.assertIn("progress", result.stdout)
            self.assertIn("::error title=Browser regression (probe.py)::", result.stdout)
            artifacts = path / "build-web/test-artifacts"
            self.assertIn("specific failure", (artifacts / "probe.log").read_text())
            timing = json.loads((artifacts / "probe-result.json").read_text())
            self.assertEqual(timing["exit_code"], 7)
            self.assertGreater(timing["seconds"], 0)


if __name__ == "__main__":
    unittest.main()
