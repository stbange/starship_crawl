# Web publication reliability and duration

The failed [main workflow](https://github.com/stbange/starship_crawl/actions/runs/42475281192)
ran for 85 minutes. Web compilation took 48 seconds; the web job then repeated
626 seconds of native build/tests before running its browser suites serially.
The parallel quality job spent 1,339 seconds running coverage-instrumented
campaigns and another 3,712 seconds building/running all sanitizers serially.
[The recorded step timings](before-timings.json) distinguish these costs.

The actual failure was `assert enemies` in the court browser combat loop.
Projectile hits are processed after the encounter-completion check in a game
tick. A browser state read can therefore see the last guard dead one tick before
`FRONTIER_CLEARED` is recorded. The normal-speed local run passed; a run with
four-times CPU throttling reproduced that intermediate state and passed with
the fix. Court, regional and frontier checks now wait for the actual completion
flag in that case, with the existing bounded timeout. They still require combat,
reward eligibility, exactly-once claims and compatible save/reload.

The workflow builds the ordinary site once and tests that same artifact in six
browser jobs. Only the two required native helper executables are built by the
browser jobs that use them; full native CTest remains required on Linux and
Windows. Coverage and sanitizers keep their existing instrumentation, thresholds,
seeds, journeys and assertions, with the six long phase cases scheduled separately.
Six coverage-campaign groups and six sanitizer groups run in separate workspaces.
Each job runs its tests serially; parallel jobs have separate filesystems and
software-GPU processes.

The unit-only coverage report is generated and retained before the campaign
workers run. Campaign workers cannot alter that report. The workflow matrices are generated from the validated assignment. The CI assignment tool
checks exhaustive/disjoint membership against the browser files and actual CTest
inventory. It fails if an assignment is missing, duplicated or stale. All groups
remain required for publication, including documentation-only main commits.

## Validation status

The normal and CPU-throttled court runs passed, including the previously failing
intermediate state. All five CI-tool self-tests passed. The split native build
registers 52 tests: 29 unit checks and 23 campaign cases. Its test-executable
archive was extracted into a fresh build directory and every registered command
resolved with executable permissions intact. A coverage-instrumented shelter
journey also executed successfully from the extracted archive without its build
cache. All three affected browser suites passed, along with the additional
CPU-throttled court run. All six independently selected phase expeditions passed.
Unit-only coverage remains 10,259/10,471 lines (98.0%), with every core file at
least 95%; all 29 unit tests and seven coverage-gate self-tests passed.
[The local results](local-checks.json) retain the exact case names.

The [complete repaired workflow](https://github.com/stbange/starship_crawl/actions/runs/42520975264)
passed at commit `f7ece01` and deployed both Pages targets on 2026-11-08 at
21:54 Europe/Warsaw. It took **18 minutes 8 seconds**, compared with the preceding
failed run's **85 minutes 31 seconds**: 78.8% less elapsed time in this comparison.
Web compilation and packaging took 36 seconds. All six browser groups (18 suites),
both native platforms, all twelve instrumented groups, specifications, unit
coverage and the separate Quest job passed. The
[recorded results and step timings](after-timings.json) include the successful
Pages deployment. This verifies the actual tested artifacts through GitHub's
deployment result; direct live-site HTTP access is unavailable under this
environment's network policy. Timings will vary with hosted runner performance.

The shipped game sources, save format and generation versions are unchanged.
Physical-device play is outside this CI-only repair.
