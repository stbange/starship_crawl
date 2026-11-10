# Coherent expedition implementation evidence

This is an ongoing implementation record for E1–E8, not a completed release report.

## E1 baseline — 10 November 2026

Three independent 30-minute **simulated** expeditions used generator 15, campaign
1, narrative 3 and save 16. The driver uses the production Session fixed tick and
held input API; menu operations use production game actions. Its policy does not
grant equipment, teleport, kill enemies or edit progression. This is automated
execution, not human play or a measure of enjoyment. Separate performance samples
include concurrent work on this development machine and are not device benchmarks.

| Seed / route | Policy | Phase 2 invested | Phase 4 invested | At 30 min | Deaths | Health lost | Wall time |
|---|---|---:|---:|---:|---:|---:|---:|
| 713 / inward | offense | 19 | 25 | 68 | 0 | 0 | 256.6 s |
| 4 / down tour | survival | 19 | 27 | 55 | 0 | 0 | 282.0 s |
| 14 / east inward | toolbox | 18 | 23 | 58 | 0 | 0 | 177.2 s |

The table uses the first 30-second sample in each phase. All four boundaries
opened; ordinary guardian fights ranged from 9.8 to 30.2 seconds, with the
optional threshold elite excluded. The three routes completed 108,000 ticks each.
The original upgrade metric includes increases in maximum capacity as well as
actual restoration; do not interpret it as pure healing. These policies make
the same first reward choice, so later comparison also needs varied rewards.

`baseline/summary.json` contains starter/middle/late samples, final resource
metrics and individual guardian timings. Each run retains compressed event
traces, three checkpoint saves, executable/library/driver hashes and a full core
source manifest in `execution.json`. The control uses the original game at `43cd491` in a detached worktree. Only
the driver changes: input runs through Session, whose normal streaming budget
is two chunks per tick. An initial double-streaming adaptation was discarded;
these retained runs all use the corrected driver. Executables and libraries are
frozen separately for control and E1, with their hashes recorded.

Reproduce against commit `43cd491`, with the Session-input change in
`tests/test_hidden_journey.c` (its exact hash is recorded per run):

```sh
cmake -S . -B build-e1-baseline -DSC_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-e1-baseline --target sc_hidden_journey --parallel 4
python3 docs/reviews/2026-11-08-hidden-simulation/data/run.py \
  --binary build-e1-baseline/sc_hidden_journey --runs 0 4 5 \
  --seconds 1800 --jobs 2 --output /tmp/starship-e1-baseline
```

New rules are being developed as generator 16 / campaign 2 / checkpoint 17.
These revisions have not yet been released. Prior ships keep their original
upgrade recovery, marginal growth, encounters and module effects.


## E1 matched result

| Seed / route | Phase 2 invested | Phase 4 invested | At 30 min | Deaths | Health lost | Wall time |
|---|---:|---:|---:|---:|---:|---:|
| 713 / inward | 11 | 15 | 29 | 0 | 2.1 | 259.9 s |
| 4 / down tour | 11 | 15 | 26 | 0 | 7.7 | 270.7 s |
| 14 / east inward | 12 | 13 | 26 | 1 | 286.0 | 180.7 s |

All three opened all four boundaries, ran 108,000 ticks and saved without failure.
The purchase-recovery metric is zero in all new runs. The direct policies reach
the last boundary with only 13–15 invested points, below the design's preliminary
30–40-point main-expedition hypothesis. At 30 minutes they have 26–29. This is a
balance observation to revisit with the integrated districts and E8, not a claim
that the final progression target has been met.

Seed 4 is an energy outlier: 286.68 seconds unable to afford its chosen shot,
versus 9.35 in the control. It still wins, but its policy keeps an expensive gun
instead of using the available basic weapon. E8 must compare deliberate weapon
switching and adjust resource pacing if the stall persists. Seed 14 dies once;
all deaths and resource losses remain in the raw trace. These runs are not the
isolated build demonstrations: those fixtures explicitly grant prerequisite
ownership and exercise three recipes at three progression levels through held
Session input, without stat changes during a fight. All nine fixtures win.

### E1 verification and captures

- Strict current/archived OpenSpec validation passes.
- 38 unit tests including Quest core pass; unit-only line coverage is
  **11,415/11,637 (98.1%)**, every executable core file >=95%.
- Separate hidden-simulation and staged-intro gameplay journeys pass.
- Native GL captures of relay, echo, charged mech and unchanged arrival pass on
  SDL offscreen/Mesa llvmpipe (80 frames each).
- Packaged WebGL branch actions, preview-before-equip, fresh reset and onboarding
  pass. Mobile refit and isolated Quest checks are recorded when finished below.

The `captures/` directory contains the three distinct branch actions and fresh
arrival. These are automated rendering observations, not human combat readability
or physical Quest comfort/performance results. Those sessions remain pending.
Intermediate E1 checkpoint-17 artifacts require the corresponding frozen E1
executable; unpublished revision 17 continues to evolve until E8 release.


E1 final input gate: mobile preview/equip and fresh reset pass after fixing the
HTML drawer's dynamic action refresh. The complete isolated Quest browser suite
passes startup/framebuffer recovery, controller input, earned menus, onboarding,
shelter/matrix/rig transitions and storage isolation. New guardian work then adds
an explicit current-format Quest encounter check rather than relying only on
legacy imported ships.

## E2 guardian gate

The two patterns each win prepared and unprepared starter-carbine Session fixtures:

| Pattern | Preparation | Simulated seconds | Deaths | Ending health |
|---|---|---:|---:|---:|
| Suspended furnace | None | 22.75 | 0 | 100 |
| Suspended furnace | Coolant bypass | 19.98 | 0 | 100 |
| Infested relay | None | 14.75 | 0 | 100 |
| Infested relay | Root bypass | 14.58 | 0 | 100 |

These isolated encounter tests use a continuously strafing, accurately aiming
policy. Their lack of health loss is not a human-difficulty measurement. They
prove the starter kit can win and do not settle E8 balance. No attributes change
during a fight; assemblies grant no progression. The separate staged-intro
journey also passes against the new rules.

39 unit tests including Quest pass. Unit-only core coverage is **11,611/11,835
(98.1%)**, every executable core source >=95%; guardians are 169/171 (98.83%).
Tests cover optional physical/matrix preparation, same-fact duplicate prevention,
co-op participation, interrupted attempts, active checkpoint staging, exact live
snapshots, transactional rejection of inconsistent assembly snapshots, strong
legal damage, finite projectiles and support preservation on legal floor/cover
positions for walking, mech and wagon shapes. Native four-scene rendering and
packaged WebGL actual assembly shooting/destruction and breach captures pass.
The gantry moves overhead; occupied floors never disappear. Inspection also fixed
a pre-existing double camera-origin subtraction in guardian marker rendering.

Current-format staging fixtures are explicit setups, generated by
`sc_guardian_tests --fixtures`. They grant no weapons or stats and remain distinct
from the input-driven victory traces. Human readability and physical Quest
comfort/performance sessions remain pending. Quest stereo encounter results are
appended after their run.


`e1-replay-linux-x86_64.tar.gz` retains the two headless executables matching the
recorded SHA256 values, so the exact intermediate E1 comparison remains runnable
after subsequent unpublished rules evolve. Extract it on Linux x86_64 with glibc
and use either executable with the saved command arguments (change only output
paths). These are simulation binaries, not game distributions. The control can
also be rebuilt from `43cd491` plus the retained driver adaptation; every compiled
core source hash is in each run's manifest. Later review builds do not replace
this pair.


E2 final platform gate: both current-format staging fixtures import into the
isolated Quest build, engage through the right grip and render nonblank, distinct
stereo eye views. This is emulated WebXR with real WebGL, not a physical Quest run.
Strict specifications, browser-inventory validation and CI-tooling tests pass.
The two new browser suites remain in opt-in extended CI; target builds and Pages
retain the lighter default workflow.


## E3 — deterministic Foundry slice

Forty unit CTests pass. Unit-only core coverage is **11,905 / 12,144 (98.0%)**;
every executable source is at least 95%, including `districts.c` at 239/251
(95.22%). The separate fresh-character staged journey passes in 47.63 seconds
wall time. Tests sample 120 regional contracts, 24 populated first vistas, six
reverse loading pairs and eight actual basic-character stair traversals. They
also exercise earned rig return travel, live/checkpoint persistence, eviction,
shared power, conductor shooting, warning duration and disconnected safe water.

Native/WASM fingerprints agree with district identities included. The packaged
WebGL suite uses real movement and G input to operate the pump and reopened
return conduit. Native Mesa captures cover the vista, raised approach, conductive
warning and open return frame. Fixed caps remain: two chunk jobs/tick and two
uploads/frame, 20 room actors, six chunk lights, 32 sprites, 32 dressing elements
and 512 boxes. Required cross passages and history reservations survive fallback.
The implementation caught and fixed stairs encroaching on the central cross
passage and an inconsistent sprite atmosphere tag before this gate passed.

Captures: [entrance](captures/coherent-foundry0.png),
[raised route](captures/coherent-foundry1.png),
[conductor warning](captures/coherent-foundry2.png),
[return conduit](captures/coherent-foundry3.png),
[pump](captures/coherent-foundry-pump.png).
These are explicitly staged inspection fixtures, not human play sessions.

The updated district geometry also passes both imported guardian encounters in
the isolated Quest build, including actual grip input and stereo captures.
Physical Quest performance and human route/readability reviews remain pending.


## E4 — a remembered repair and an interrupted supply line

The unit paired campaigns retain the same initial source, destination and force:
without intervention, the supply force reaches a siege and changes its target’s
owner; after two members are killed, the survivor retreats and the original owner
remains. The factory variant repairs production and creates force 2 at a second
normal resource cost. Interruption prevents that reinforcement. NPC combat adds
no player XP or kills. See [paired outcomes](e4-paired-campaigns.txt).

Two separately labelled post-first-guardian fixtures use Session movement,
aiming, shooting and decisions after setup. Taking salvage leaves an evacuation
note at [3,0,0]; repairing the refuge lets the same named engineer travel to
[6,0,0] and acknowledge the earlier help. Both journeys walk to the bay, simulate
ten minutes elsewhere, save/import and walk to the later record. They complete
in 740.65 and 804.38 simulated seconds, with 11 kills and zero deaths each. The
final separate gameplay CTest takes 20.88 seconds wall time on this container.
These are controlled automated journeys, not fresh-character or human sessions.
The [retained transcript](e4-revisit-ctest.txt) distinguishes their setup and events.

Final E4 gate: **41 unit CTests pass**, with unit-only coverage **12,297/12,553
(98.0%)**, every executable core file >=95%. Individuals are 203/208 (97.60%),
campaign chains 120/121 (99.17%), and the checkpoint appendix 29/29 (100%).
Strict current/archived specifications, CI inventory and five tooling tests pass.
Native Mesa, packaged WebGL keyboard/controller/touch and isolated Quest grip/menu
stereo checks pass. Visual inspection reduced overly large close-range labels
and replaced a misleading generic preparation hint with the actual salvage cost.
The malformed-onboarding test now writes invalid semantic fields through the real
serializer; its old end-of-file offsets could target unrelated appended fields.
Both generator-15 and generator-16 rejection paths are checked transactionally.

[Engineer](captures/coherent-engineer.png),
[known costs](captures/coherent-refuge-options.png),
[evacuation aftermath](captures/coherent-refuge-abandoned.png),
[touch choice](captures/coherent-mobile-refuge.png),
[Quest emulation](captures/coherent-refuge-stereo.png).
Human judgments about emotional impact and physical Quest testing remain pending.

## E5 — sanctuary preparation

The three optional meals, shared refit/calibration/research projects and six
keepsake shelves are implemented. Tests exercise zero-resource bed/medical/stew
recovery, personal replacement and paused timers, safe point conservation,
unchanged equipment eligibility, stale co-op quotes, calibrated flare attachment,
actual-presence observation, overflow field records, new-ship clearing and
transactional persistence. This adds no mandatory maintenance to shelter visits.

The native Mesa fixture shows the locker-side project bench; WebGL checks use
real keyboard, controller and touch actions. Quest checks import a labelled
post-guardian fixture, use the grip/menu controls and draw both eyes. These are
automated fixtures, not physical-headset or human usability results. The paired
inhabitant revisit and guardian gameplay tests also pass (22.89 seconds wall).

Final E5 gate: 42 unit CTests pass; unit-only coverage is **12,572/12,831
(98.0%)**, with every executable source >=95%. The preparation logic and its
checkpoint appendix both measure 100%. Strict specifications, snapshot schema,
CI inventory and tooling checks pass. The ordinary WebGL facility suite passes
in 75.2 seconds and isolated Quest emulation in 13.4 seconds. Native capture
uses the explicit `home2` inspection scene; an earlier smoke invocation with a
save directory deliberately skipped loading because smoke mode disables saves,
and is not counted as facility evidence. The browser harness now accepts the
existing save-import confirmation.

[Zero-resource choices](captures/coherent-home-zero.png),
[prepared meal](captures/coherent-home-meal.png),
[project research](captures/coherent-home-projects.png),
[native bench](captures/coherent-home-projects-native.png),
[touch](captures/coherent-home-mobile.png),
[Quest emulation](captures/coherent-home-meals-stereo.png).
