"""Check that the coverage gate fails closed and enforces the requested boundary."""
import importlib.util
from pathlib import Path
import unittest
import tempfile

path = Path(__file__).resolve().parents[1] / "scripts" / "check-coverage.py"
spec = importlib.util.spec_from_file_location("coverage_gate", path)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class CoverageGateTests(unittest.TestCase):
    def report(self):
        return {"files": [{"file": name, "lines": [{"count": 1}] * 20}
                          for name in sorted(gate.required_sources())]}

    def test_data_table_macros_are_allowed_but_functions_are_rejected(self):
        original = gate.ROOT
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "cmake").mkdir()
            (root / "src").mkdir()
            (root / "cmake/CoreSources.cmake").write_text(
                "set(SC_CORE_SOURCES src/content.c src/session.c)\n"
                "set(SC_CORE_DATA_SOURCES src/content.c)\n")
            source = root / "src/content.c"
            try:
                gate.ROOT = root
                source.write_text("#define TABLE(name) {name, 1}\nconst int table[] = TABLE(2);\n")
                self.assertIn("src/session.c", gate.required_sources())
                source.write_text("int gameplay(void) { return 7; }\n")
                with self.assertRaises(SystemExit):
                    gate.required_sources()
            finally:
                gate.ROOT = original

    def test_empty_report_is_rejected(self):
        with self.assertRaises(SystemExit):
            gate.validate_report({"files": []})

    def test_exactly_95_percent_is_accepted(self):
        report = self.report()
        for source in report["files"]:
            source["lines"][-1] = {"count": 0}
        gate.validate_report(report)

    def test_one_low_file_fails_even_when_overall_passes(self):
        report = self.report()
        report["files"][0]["lines"][-2:] = [{"count": 0}, {"count": 0}]
        with self.assertRaises(SystemExit):
            gate.validate_report(report)

    def test_new_core_file_is_also_measured(self):
        report = self.report()
        report["files"].append({"file": "src/new_logic.c", "lines": [{"count": 0}]})
        with self.assertRaises(SystemExit):
            gate.validate_report(report)

    def test_missing_required_source_is_rejected(self):
        report = self.report()
        report["files"].pop()
        with self.assertRaises(SystemExit):
            gate.validate_report(report)

    def test_quest_gate_requires_adapter_and_measures_it(self):
        report = self.report()
        with self.assertRaises(SystemExit):
            gate.validate_report(report, require_quest=True)
        report["files"].append({"file": "src/quest.c", "lines": [{"count": 1}] * 20})
        gate.validate_report(report, require_quest=True)
        report["files"][-1]["lines"][-2:] = [{"count": 0}, {"count": 0}]
        with self.assertRaises(SystemExit):
            gate.validate_report(report, require_quest=True)

    def test_rounding_does_not_hide_a_failure(self):
        report = self.report()
        report["files"][0]["lines"] = [{"count": 1}] * 1899 + [{"count": 0}] * 101
        with self.assertRaises(SystemExit):
            gate.validate_report(report)


if __name__ == "__main__":
    unittest.main()
