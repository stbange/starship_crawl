# Quest WebXR target verification

This change adds the opt-in `starship_crawl_quest` application and a separate
`/quest/` Pages entry. It starts from main commit `aa12477`. The ordinary Linux,
Windows and browser applications retain their existing core, entry points,
settings and save storage. See [QUEST.md](../../../QUEST.md) for controls and
build instructions.

## Isolation evidence

Preprocessing the modified game, matrix, platform entry point and renderer with
VR disabled yields the same C token sequences as the starting commit, including
their headers and renderer includes. The
[recorded comparison](ordinary-source-isolation.json) covers those four translation
units. Ordinary native/web compile definitions contain no `SC_WEBXR`; the Quest
core alone receives `SC_WEBXR=1`. Ordinary JS contains no `sc_xr_begin` export.
The packaged ordinary HTML, JS and WASM are byte-identical when staged alongside
the separate Quest subdirectory.

A 20-second ordinary-control expedition compiled against each core produces
byte-identical saved files. Browser checks exercise different saved seeds and
comfort settings on the same origin. VR imports use a temporary file outside
IDBFS, avoiding a race between deletion and asynchronous persistence.

## Executed local checks

| Check | Result |
| --- | --- |
| Native Linux CTest | 44/44 passed |
| Separate gameplay-labelled expeditions | 19/19 passed |
| Quest tracking/combat, ordinary core and saved-state parity | 5/5 passed |
| Quest address, undefined-behavior and leak sanitizers | 5/5 passed |
| Unit-only core coverage, including Quest variant | 10,259/10,471 lines, 98.0%; every measured file at least 95% |
| Tracking adapter coverage | 103/103 executable lines, 100% |
| Strict specifications, coverage-gate and packaging checks | Passed |
| Full WebXR emulation suite | Passed: entry failures, stereo output, aiming, controls, lifecycle, progression, facilities and save isolation |
| Ordinary packaged browser suites | All 18 passed |
| Native OpenGL render checks | Eight scenes passed, including six split-screen scenes |
| Ordinary Windows cross-build | Passed with MinGW-w64; VR options remain off |
| Clean Quest test dependencies | Pinned requirements installed and imports verified with Python site packages disabled |
| Quest CI job, pinned Emscripten 4.0.23 | Passed: build, packaging, full stereo/browser suite and downloadable artifact |
| Windows-hosted execution and complete Pages publication | Required CI verification pending |

The [coverage record](core-coverage.json) lists every measured file. Renderer,
platform and browser bridges use integration tests, as in the existing project;
the coverage figure describes the headless core, not the entire application.
Local LeakSanitizer runs require execution outside this cloud sandbox's tracing
layer. The gate itself remains enabled, with no coverage exclusions.

The external test device supplies standard WebXR poses, Touch-style controllers,
sessions and a real WebGL framebuffer. The application renders both eye views
and advances the simulation once per XR frame. Tests use controller actions and
normal save imports; they do not introduce production game-state setters.

The complete suite covers fresh-character boarding; independently aimed shots;
snap-turn release latches; paused head tracking; floating and wrist maps;
controller, visibility and head-tracking loss; release-plus-A recovery; session
end/re-entry; separate same-origin storage; compatible imports; earned guardian
rewards and upgrades; shelter, matrix, mech and wagon transitions; actual
WebGL context loss and a reload path. The final release run also passed an actual
story decision and exercised the expanded loadout/decision text in the floating
panels. Retained results list the [nine VR check groups](quest-browser-results.json),
[eighteen ordinary suites](ordinary-browser-suites.json) and
[native render scenes](native-render-checks.txt).

The implementation is in commits `3c7f9d0` and `8bff083`. The first CI attempt
identified an undeclared Pillow dependency in the new image-comparison test.
The correction pins it in `tests/requirements-quest.txt`, used only by the Quest
job; a clean temporary Python installation verified those imports. The
[corrected Quest job](https://github.com/stbange/starship_crawl/actions/runs/42474447478/job/128712513180)
passed every step and uploaded `quest-webxr`; its [step results](ci-quest-job.json)
are retained here. The complete workflow is still running at this record's update.
Publication and Windows-hosted execution are not yet claimed. The release task
remains open until the full workflow and Pages deployment are confirmed.

## Stereo captures

These are real framebuffer captures from the software test headset, **not
photographs or performance measurements from a physical Quest 3**. Each image
contains the left and right eye at 640 × 640 pixels per eye.

[Space approach](approach-stereo.png) · [Raised wrist map](wrist-map-stereo.png) ·
[Shelter interior](shelter-stereo.png)

## Physical validation

Quest 3 comfort, sustained frame rate, text readability, audio and controller
ergonomics remain pending. The runtime requests 72 Hz only when advertised;
that request is not a measured frame-rate result. Follow the
[hardware checklist](../../playtests/QUEST3_CHECKLIST.md) before making those
claims or deciding which rendering optimizations are necessary.

Every main push, including the later release-record/specification archive,
continues to run the full workflow. The existing Pages root remains deployed
until the quality, native, ordinary-web and Quest jobs all succeed.
