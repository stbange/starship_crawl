# Tasks

## 1. Optional workflow

- [x] 1.1 Separate fast checks from extended verification and preserve target/artifact jobs; verify the workflow passes actionlint and default-off input inspection.
- [x] 1.2 Make Pages independent of skipped optional dependencies while requiring selected checks to succeed; verify normal/manual, failure, skip, cancellation and non-main cases using GitHub's expression evaluator.

## 2. Documentation and validation

- [x] 2.1 Update repository instructions, README, testing/Quest docs and the active expedition plan; verify the documented workflow controls match the YAML and coverage thresholds remain unchanged.
- [x] 2.2 Run strict OpenSpec, snapshot-schema verification, existing tooling self-tests, browser inventory and Quest packaging checks; retain successful command results before commit.

## Workflow follow-up

- Archive the completed CI change, push main and observe the automatic target
  builds, skipped optional checks, downloadable artifacts and Pages deployment.
