"""Check that the coverage gate fails closed and enforces the requested boundary."""
import importlib.util
from pathlib import Path
import unittest

path = Path(__file__).resolve().parents[1] / "scripts" / "check-coverage.py"
spec = importlib.util.spec_from_file_location("coverage_gate", path)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class CoverageGateTests(unittest.TestCase):
    def report(self):
        return {"files": [{"file": name, "lines": [{"count": 1}] * 20} for name in (
            "src/game.c", "src/generation.c", "src/history.c", "src/interior.c", "src/world.c", "src/progression.c", "src/story.c", "src/expedition.c", "src/death.c", "src/save.c", "src/shelter.c", "src/navigation.c", "src/psionics.c", "src/equipment.c", "src/matrix.c", "src/matrix_gen.c", "src/matrix_state.c", "src/matrix_effects.c", "src/campaign.c", "src/campaign_gen.c", "src/campaign_combat.c", "src/math3d.h") ]}

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

    def test_rounding_does_not_hide_a_failure(self):
        report = self.report()
        report["files"][0]["lines"] = [{"count": 1}] * 1899 + [{"count": 0}] * 101
        with self.assertRaises(SystemExit):
            gate.validate_report(report)


if __name__ == "__main__":
    unittest.main()
