# Staged onboarding verification — 9 November 2026

The introduction starts with a carbine and navigation, then awards the shotgun,
shelter, matrix and later capabilities through combat and exploration.
[Behavior and controls](../../../ONBOARDING.md).

The retained ordinary-input expedition starts in space at seed 713, earns its
interface at `(4,0,2)`, restores the physical room's power, opens all four phase
boundaries and extracts after 975.10 simulated seconds. It fires 777 shots,
records 119 kills and zero deaths, with 452 projectile hits and a minimum shield
of 22. This automated controller has a 0.30-second reaction delay, a 2.8-radian/sec
turn limit and 0.025-radian aim error. It demonstrates reachability and persistence;
it is not a human pacing or difficulty measurement.

Reproduce with `build/sc_phase_journey --onboarding OUTPUT_DIRECTORY`.
The input driver contains no position, health, inventory or enemy grants. Its
`phase-interface.sav` is retained as `tests/fixtures/onboarding-interface.sav`
for browser and Quest tests, with its checksum in the fixture provenance.

The native component/platform suite and all 20 gameplay cases pass. All 30
sanitizer unit cases and the introductory expedition pass. Unit-only core line
coverage is 98.0%, above 95% for every measured source; presentation adapters are
validated separately through rendering and packaged browser input.

All 19 ordinary packaged-browser suites pass, including legacy matrix imports.
The new WebGL suite passes actual keyboard, virtual-standard-controller and touch
interactions, progressive menus, Guided/Minimal/Off persistence, saved character
lessons, old-save access and power restoration. Full Quest regression exercises
stereo frames, tracking recovery, menus, ordinary/VR storage isolation and earned
legacy facility journeys. The focused new Quest case uses the earned interface
checkpoint and right-grip interactions. No physical headset was tested.

The final guidance check verifies that the matrix lesson takes priority while
linked and that upgrades name the correct keyboard, controller, touch and Quest
controls. The reviewable field guide includes those upgrade controls.

Captured directly from the compiled game:

- [One-weapon arrival and contextual guidance](onboarding-arrival.png).
- [Reviewable field guide](onboarding-field-guide.png).
- [First matrix link](onboarding-first-link.png).
- [Return to the restored physical room](onboarding-restored-power.png).
- [Mobile opening controls](onboarding-mobile.png).
- [Quest introductory link](quest-onboarding-first-link.png) and [restored room](quest-onboarding-restored-power.png).

The retained logs provide local evidence. GitHub Actions runs the complete native,
coverage, sanitizer, packaged-browser and Quest gates again before publishing.
Manual follow-up is whether a first-time player notices the bay objective, learns
shooting before choosing upgrades, understands the visible effect of restoring
power, and can find/disable guidance comfortably on their own device.
