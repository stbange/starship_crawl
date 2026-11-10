# Optional extended CI

## Context

The existing workflow runs coverage, sanitizer campaigns, full native CTest and
ordinary/Quest browser suites on every commit. The latest documentation-only run
took roughly twenty minutes before deployment. Target builds themselves finish
earlier. This change implements the user's explicit scheduling preference.

## Goals / Non-Goals

Keep every shipping target and artifact automatic, make lengthy verification an
explicit manual choice, and preserve failure propagation when it is chosen.
Test inventories, assertions and the 95% unit coverage threshold do not change.
No gameplay, dependency, format or generator changes are needed.

## Decisions

Use one default-off boolean `workflow_dispatch.inputs.extended_tests`. Push and
pull-request events have no input and use the normal build path. A single switch
keeps the full verification reproducible without maintaining partial combinations
of coverage/sanitizer/browser groups.

Keep the `quality` job for short checks. Move coverage and its packaged test
executables/output matrix into a gated `coverage` job. Gate `sanitizer-build`,
`instrumented` and `web-browser` jobs; the instrumented matrix depends on the
coverage output. Native builds set `BUILD_TESTING=OFF` by default and enable it
with the full CTest/render steps only when requested. Quest packaging remains
automatic; its browser installation and suite become conditional steps.

Pages assembly depends directly on the ordinary and Quest builds, so skipped
browser jobs cannot skip the assembly. Deployment uses an explicit status
function to avoid GitHub's implicit `success()` condition on skipped dependencies.
It requires successful builds/fast checks and either all optional jobs skipped
in normal mode or all optional jobs successful in extended mode. A requested but
unexpectedly skipped test fails closed. Pull requests, non-main refs and cancelled
runs cannot deploy.

Alternative considered: publish independently of selected tests. Keeping selected
tests as gates makes the manual verification useful for release decisions. A
second workflow would add artifact/run coordination without helping this scope.

## Risks / Trade-offs

- Normal publication has less gameplay regression evidence. All suites remain
  runnable locally or through the explicit extended run; feature-validation
  instructions still require appropriate testing.
- Conditional job dependencies can silently skip publishing. Validate the actual
  expressions against default/manual/failure/cancellation contexts and observe a
  real default main run.
- An empty skipped coverage matrix must not be expanded. The job-level condition
  is evaluated before its matrix, and both scheduling modes are checked.

## Migration Plan

No data or application migration. The next main push uses the default build path.
Choose Actions → Build, test, and publish → Run workflow → Run extended tests for
the full suite. Existing repository settings and Pages permissions remain valid.
If an administrator has independently made optional job names required branch
checks, adjust those repository rules separately; do not change remote settings
without evidence that such a rule is blocking work.

## Local verification

Actionlint 1.7.7 accepts the workflow. The official `@actions/expressions` 0.3.61
evaluator passes 171 checks against its actual YAML conditions: push/PR/manual
defaults, explicit opt-in, native/Quest test-step selection, dependency failures,
unexpected skips, cancellation, non-main refs and the coverage matrix producer.
Those tools were installed only in a temporary validation directory; project
dependencies did not change. Strict OpenSpec, snapshot-schema verification, all
eight coverage-tool self-tests, five CI-tool self-tests, Quest packaging tests and
the browser inventory check pass. These checks do not claim a fresh core coverage
measurement or an executed extended game suite. The automatic main build and
publication are checked after pushing.
