# Verification record

These logs distinguish compilation, automated simulation, software rendering
and physical-device testing. The latter has not been performed in this cloud
workspace. No test result below substitutes for human difficulty/readability
or listening to the sound mix.

| Gate | Result / evidence |
| --- | --- |
| Strict specifications | 22 current specifications and 20 archived changes pass after archiving, [final log](archive-specs.log); [earlier log](verified-specs.log) retained |
| Coverage gate self-tests | 6/6, [log](verified-quality-tools.log); deliberate bad-report fixtures print expected rejection messages |
| Unit and SDL platform checks | 25/25, [JUnit](hidden-final-unit-platform.xml) |
| Full final native release suite | 44/44 (24 unit, 19 gameplay, 1 SDL platform), 575.97 seconds, [JUnit](hidden-release-native.xml), [log](release-native.log) |
| Separately run gameplay label | 19/19, [JUnit](hidden-final-gameplay.xml), 695.69 seconds |
| Unit-only core coverage | 10,119/10,334 lines, 97.92%; every source >=95%, [gate](warning-coverage.log), [per-file summary](unit-coverage-summary.txt), [XML](unit-coverage-coverage.xml) |
| Five new suites with ASan/UBSan and leak detection | 5/5 outside the ptrace sandbox, [results](leaks-results.json) |
| Full ASan/UBSan suite | 43/43, 3,804.33 seconds, [JUnit](sanitizer-final.xml), [log](sanitizer-final.log); leak detection disabled for this sandboxed run |
| Coverage-build gameplay profiles | All four profiles pass in 623.20 seconds with the twenty-minute allowance, [JUnit](coverage-profiles-final.xml), [log](coverage-profiles-final.log). The earlier ten-minute timeout is retained; unit-only coverage above is unaffected. |
| Packaged browser suites | All 18 pass together in the final [CI web job](ci-published.json); local [batch](browser-results.json) and [reruns](browser-final-results.json) retained |
| Linux OpenGL smoke captures | 5/5, [commands/results](../native-render/results.json); inspection scenes, separate from the earned WebGL encounters |
| Windows x86-64 cross-build | Passed, [log](release-windows.log); no Windows graphics/device session |
| Emscripten release build/package | Passed, [build](warning-web-build.log), [package](warning-package.log) |
| Long-run save validation | All 428 completed-run checkpoints load, [record](../data/checkpoint-validation.json) |
| Complete main-branch CI and Pages publication | Passed for `212fe1e`, including quality, both native jobs, web and deployment; [run](https://github.com/stbange/starship_crawl/actions/runs/42440873874), [record](ci-published.json) |

The first fourteen successful browser suites use the stable package preceding
the final director tuning/stereo changes. Foundation, guidance, main and hidden
factions were then run against the corrected release. The final warning-cue
capture exercises the subsequent presentation refinement. Source/WASM manifests
travel with the capture folders. The final CI web job passed every suite against
the same pushed revision.

The full native release rerun includes the final warning presentation. The full sanitizer
run uses the four-metric journey driver; the final six-metric driver changes
only review timing wrappers, and completed its own 600-second expedition.
The later warning refinement changes presentation and a warning string; the
simulation, persistence and progression rules are unchanged.

## Commands and retained failures

The normal commands are documented in [TESTING.md](../../../../TESTING.md).
The local release checks use `ctest --test-dir build --output-on-failure`,
`ctest --test-dir build -L gameplay --output-on-failure`, and
`python3 scripts/check-coverage.py`. Sanitizers use Debug with `-O1 -g` and
`SC_SANITIZE=ON`; the sandboxed full run sets `ASAN_OPTIONS=detect_leaks=0`.
The five focused binaries were also executed with leak detection enabled
outside the sandbox. Coverage is collected only from unit-labelled tests.

The earlier [native run](verified-native.log),
[campaign candidate](final-native-tests.log),
[sanitizer attempt](sanitize-tests.log),
[foundation](browser-foundation.log) and [guidance](browser-guidance.log)
failures are kept alongside the passing reruns. The main review explains the
guardian-footprint save rejection, invalid legacy test fixture and software
rendering deadlines; none is silently counted as a pass. The
[legacy timing result](legacy-fixture-time.json) measures the corrected fixture.

`footprint-web.log` contains a successful build followed by an invalid packaging
argument. `footprint-package.log` records the corrected invocation. Later
`warning-web-build.log` and `warning-package.log` are the final presentation build.
The capture-driver attempts are retained in separate `warning-capture-*`
directories rather than overwritten by the successful run.

The [first implementation workflow](ci-first-attempt-final.json) finished with
both native jobs and the complete web job passing. Its coverage-build campaign
timeout prevented deployment. The [corrected workflow](ci-published.json) passed
all jobs and published the tested artifact. Publication was verified through
the successful official Pages deployment and its project URL; direct live-site
HTTP access is blocked by the cloud proxy. The packaged browser checks exercise
the same artifact before deployment.

This completion record and OpenSpec archive are a documentation-only follow-up.
Their push triggers another full workflow, as required by the project; the
published gameplay revision above has already passed all checks.
