# Project workflow

Starship Crawl is C17 with SDL3/OpenGL and an Emscripten WebGL build. Read README.md,
TESTING.md, openspec/config.yaml, and the relevant openspec/specs before changing behavior.

OpenSpec is a pinned project-local development dependency. Run `npm ci --ignore-scripts`.
Use `npm exec -- openspec ...` for commands shown as `openspec ...` in the generated
`.agents/skills/openspec-*` integrations. Feature and behavior changes should have an
OpenSpec change; routine fixes can use the already authorized task scope.

Keep the shipped core independent of test hooks. Fault injection belongs only to
`sc_core_fault`. Quality-only work must preserve save/generator compatibility.

Run `npm run spec:validate`, native CTest, and `python3 scripts/check-coverage.py` for
core changes. Coverage is unit-only core line coverage: >=95% overall and per file,
with no exclusions of uncovered core lines. Run the gameplay-labelled campaign tests
separately. For renderer, platform, web, or input changes also run the packaged browser
suite and relevant render smoke checks. See TESTING.md for exact commands and scope.

Report compilation, automated execution, and physical-device testing accurately.
