## MODIFIED Requirements

### Requirement: Publication gates

Pages publication SHALL require successful native, ordinary-web and Quest builds,
fast specification/tooling checks and packaging/startup checks. Long verification
SHALL be optional. A run that selects it SHALL require every selected job to
succeed. Only uncancelled non-pull-request runs on main SHALL publish.

#### Scenario: Failed verification

- **WHEN** any required build, fast check or selected extended check fails
- **THEN** Pages is not deployed and available test or coverage diagnostics are retained.

#### Scenario: Publish a normal main push

- **WHEN** a push to main passes all required builds and fast checks without selecting extended tests
- **THEN** skipped optional jobs do not prevent assembly and deployment of both web targets, and native/web download artifacts remain available.

#### Scenario: Selected verification is skipped

- **WHEN** an extended run has an unexpectedly skipped or cancelled selected verification job
- **THEN** the run does not publish Pages even if its target builds succeeded.

#### Scenario: Build another ref

- **WHEN** a pull request or non-main ref builds successfully
- **THEN** its build artifacts remain available and Pages is not deployed.

## ADDED Requirements

### Requirement: Explicit extended verification

Manual workflow runs SHALL offer a boolean extended-test input defaulting off.
Normal pushes and pull requests SHALL build all shipping targets with fast checks
without starting full native/gameplay/render suites, coverage, sanitizer campaigns
or browser sessions. Selecting the input SHALL run those existing suites with
unchanged inventories, assertions and coverage thresholds.

#### Scenario: Choose full verification

- **WHEN** a contributor starts a manual run with extended tests enabled
- **THEN** full native and rendering tests, at least 95% per-source and overall unit coverage, sanitizer campaigns, all ordinary browser suites and the Quest browser suite run.

#### Scenario: Another push after manual verification

- **WHEN** a push follows a manual extended run
- **THEN** it uses the default build path without inheriting the previous run's extended-test choice.
