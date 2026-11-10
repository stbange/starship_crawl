# Proposal

## Why

The full verification workflow delays every build and Pages update, including
documentation changes. The user requests optional long-running checks so routine
commits can publish all targets promptly.

## What Changes

- Keep Linux, Windows, ordinary web and Quest builds and downloadable artifacts
  automatic on pushes, pull requests and manual runs.
- Keep short specification, tooling, packaging and Linux startup checks automatic.
- Add a manual `extended_tests` boolean, default false, enabling full native and
  rendering tests, unit coverage, sanitizer campaigns and all browser suites.
- Permit skipped optional jobs to publish on main while requiring every selected
  extended check to succeed. Failed builds and cancelled runs cannot publish.
- Update workflow instructions and the active expedition plan to use this policy.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `quality-gates`: separate automatic build/publication gates from optional long
  verification while retaining the existing coverage thresholds and test scope.

## Impact

Changes affect CI configuration and its documentation. Game code, dependencies,
save format, generators, target isolation and test assertions remain unchanged.
Validate workflow syntax, actual GitHub expression conditions, existing fast
tooling tests and strict OpenSpec; observe the normal main build and publication.
