# Hidden factions and invisible director: implementation review

This review covers the six milestones in the
[OpenSpec change](../../../openspec/changes/hidden-factions-and-director/tasks.md).
The implementation is generator 14 / simulation 1 / narrative 3 / matrix 1 /
save 15. Generator-13 courts retain their format-14 path. The
[player guide](../../../HIDDEN_FACTIONS.md) describes the resulting behaviour.

All six milestones' mechanics and the long-run tuning are implemented. Native
release, separate gameplay, coverage and packaged browser/render checks pass;
the final full sanitizer run and CI/Pages publication are still in progress.
The [verification record](verification/README.md) separates the performed gates
from pending checks and retains unsuccessful attempts.

## Protocol and evidence

Twelve independent characters use seeds 713, 42, 1, 3, 4, 14, 22, 109, 314,
2026, 9001 and 8675309. Each runs **3,600 simulated seconds at 60 Hz**, using
different inward, horizontal, vertical and diagonal routes and three imperfect
input/build profiles. Four selected seeds repeat with only the director call
disabled in the test executable. These are accelerated automated simulations,
not twelve hours of human play or rendered real-time sessions.

The controller uses normal movement, look, fire, lift, interaction, upgrades,
shelter and save/import actions. It does not write positions, kill actors, grant
equipment or alter resources. Navigation reads the same route information used
by the HUD. Each expedition begins at the space approach, earns its character,
follows the guardian/story progression, extracts and continues into its chosen
direction. A save/resume occurs after thirty simulated minutes. The harness checks finite health/energy and the streaming-work budget each frame,
records resources, movement, casualties and pacing, and validates simulation
state at the end. The thirty-minute import exercises full save validation. A later read-only
check also loaded all [428 checkpoints](data/checkpoint-validation.json) from
completed runs through the ordinary loader, with no rejections.
Separate unit/stress tests check ownership, casualty/claim conservation and
co-op/cache transitions; those are not implied to be per-frame long-run checks.

- [Complete result table](data/results.md) and [machine-readable summary](data/summary.json).
- [Rendered encounter gallery](gallery.md), including real stereo audio, and
  [native/web performance measurements](data/performance.md).
- Each `data/run-XX`, `data/without-director-XX`, `data/tuned/run-XX` and
  `data/footprint-fixed/run-XX` contains
  its exact command, source/library/binary hashes, elapsed wall time, all JSONL
  events, stderr, periodic saves and earned checkpoints. Nonzero and incomplete
  outcomes remain visible.
- [Runner](data/run.py), [analysis/inspection](data/analyze.py), and
  [input controller](../../../tests/test_hidden_journey.c).
- The initial twelve runs and four controls use the pre-tuning core. Their
  differing files and exact original driver are retained in
  [pre-tuning-source](data/pre-tuning-source). Other headless-core files match the corresponding source manifest; the
  subsequently changed stereo platform adapter is not linked into these journeys. Four affected routes are repeated after director tuning.
- The tuned batch precedes the guardian-footprint fix below and has one stricter
  load-validation condition: already committed small-room nests from the earlier
  candidate remain importable. [Its candidate validator](data/tuned-candidate-validator.c)
  preserves the exact earlier source hash. New proposals still enforce clearance.

Run the controller through CMake and `data/run.py`; use `--output` with a new
directory for each repeated batch. Never overwrite a failed run with a pass.

## Findings and changes made during playtesting

The wars continue without player commissions or director stepping. Supplies pay
for formations and cargo, troops arrive and retreat, sites change hands, and
casualties persist. The ordinary-input raid journey follows a formation across
six cells, interrupts it, leaves the area, revisits, reaches an occupied station
and returns to the bay. A separate journey approaches a carrier to nine metres
through the actual doorway. Saves and logs establish the actor identities.

Two movement defects found during development were corrected: an ascending
formation oscillated at the first service stair landing, and a lift ride could
remain unfinished after its destination left the physical cache. The unit tests
now traverse the full stair and save/resume an offscreen lift handoff. The two
interrupted preliminary expeditions remain in
`data/preliminary-before-lift-fix`; they do not count as completed hour runs.

The initial director often failed to reach discovery while the player travelled
through empty rooms, because every room change reduced its quiet timer. It now
resets quiet time for observed content, remembers recent encountered families and
formation orders, avoids repeatedly redirecting a group the player left behind,
and recognises prolonged combat separately from lost health. Pending unseen
attacks cancel when recovery becomes necessary. Repetition can age out after a
quiet interval rather than exhausting an event category permanently. Unseen
encounter substitution uses the room's ecology and retains static identities,
depth scaling and confirmed deaths. These changes are covered by unit tests and
the four tuned repeats.

A tuned diagonal run then exposed a real save defect at thirty minutes: the
requested cell was inside the footprint of a protected guardian hall, but the
proposal check only recognised its canonical cell. An illegal stash was placed
and the loader correctly rejected it. Protection now canonicalises the entire
room footprint before planning, with a seed-2026 regression. The failed
`tuned/run-09` remains intact; `footprint-fixed/run-09` is the corrected repeat.
The loader was not weakened to accept the illegal guardian edit.

Final presentation review also found that the first-hit warning was mostly a
text message. It now emits a brief stronger directional call and two raised
signal bars on nearby guards during the existing grace interval. This uses the
existing incident state and changes no hostility thresholds, damage, routes or
save schema. The earlier journey/capture sources are reproducible using
[pre-warning-source](data/pre-warning-source); the final firing capture and
release checks are retained separately.

The [controlled doctrine capture](data/doctrines.jsonl) records asserted unit
fixtures, not a player expedition: the Molting Court selected biomass, Echoes
selected power, and a power shortage changed the Molting Court's selection.
Successive Crown orders selected opposite decks. Resource transport, relation
cooldowns, witnessed incidents and co-op attribution retain their bounded rules;
these runs did not justify additional aggression or a new reward multiplier.

The strong input profiles still accumulate substantial power and often take
little health damage. These results establish functional encounters and
persistence; they do not establish satisfying human difficulty. General stat
growth, recovery economy and late-game challenge remain the separate balance
work identified in the [previous review](../2026-11-07-long-expeditions/README.md).

## Long-run results

**Twenty one-hour simulated expeditions completed:** twelve baseline routes,
four director-disabled controls, and four tuned repeats (including the corrected
diagonal rerun). The rejected half-hour candidate and preliminary attempts are
retained separately. All completed runs reached phase four, saved/resumed, and
retained valid simulation state. Their deepest visited layers ranged from 9 to
50; recorded force counts and per-frame streaming work stayed within their caps.

The [paired table](data/comparisons.md) shows that disabling the director did not
stop deployments, battles or occupation: the four controls recorded 130–257
order completions and 2–16 captures each. Tuning reduced interventions from
60–65 to 42–51 per hour. Discovery-state time rose to 17–44%, while recovery-state
time remained 29–47%. Those are scheduler modes, not a human rating of pacing.
The reacting controller can take different paths after an intervention; this
is not an identical-input counterfactual after the initial seed and profile.

No deaths occurred in these strong hour-long profiles. The broader imperfect-input
campaign regressions include weaker reaction/aim profiles and actual deaths.
Power growth remains too forgiving for these automated builds. The corrected
diagonal journey also needed 47 stuck-recovery manoeuvres and skipped two
optional waypoints; it finished and resumed successfully, but this is useful
evidence for further navigation polish, not flawless traversal.

The completed runs retained 170–550 strategic region summaries. Final saves
were 43.2–75.2 KiB, with a fixed 20,064-byte faction appendix plus 72 bytes per
retained region. Native `Game` storage is 2,926,040 bytes, including 54,912 bytes
of fixed faction state. At most 21 of the 32 formation slots were live, and up
to 33 mobile members were physical across the shared room cache (not in one
room). The 20-actor per-room cap remained enforced. These measurements do not
include renderer/SDL allocations or imply a mobile-device frame-rate guarantee.

## Retained unsuccessful attempts

- `preliminary-raid-driver-target`: the controller revisited the correct room but
  checked the wrong console coordinate. The production navigation was correct;
  the driver now checks the terminal it navigates towards.
- `preliminary-raid-intercept-lost`: the selected distant raid ended before the
  player caught it. This is a valid autonomous outcome, not a successful follow.
- Two software-WebGL lift waits reached their 20-second wall deadline while
  still making valid progress. Control waits now allow 60 seconds; actual deck,
  ride completion, release/latch and independent co-op assertions are unchanged.
  Original failures and final reruns are retained.
- The cloud workspace restart interrupted the browser batch during its phase
  suite. Completed suites were retained; unfinished suites were restarted.
- Initial sanitizer runs exceeded the solo/phase campaign deadlines. The legacy
  fixture retained a new-ship simulation flag after changing its generator;
  correcting that setup reduced the measured three-campaign sanitizer run to
  13.05 seconds (all three passed). The exploratory run using the invalid
  fixture was stopped and retained. Instrumented phase runs had completed their
  first campaigns before the old deadline; their two/four-campaign wall limits
  are now 1,200/2,400 seconds, with all simulated-frame and gameplay assertions
  unchanged. Final reruns are reported separately.

## Measurement and human follow-up

The final [measured update table](data/performance.md) includes the complete game
step: native p95/p99 were 2.17/2.86 ms, and the packaged browser probe measured
10.2/25.7 ms. Browser animation-frame intervals had a 200 ms median with software
WebGL. These different scenes and execution environments do not establish a
platform-speed comparison or hardware frame-rate target. Browser linear memory
was 64 MiB with 32 retained regions; the hour runs provide the larger-save sample.

Native runs report per-frame strategy, physical-force and director timing
percentiles; the tuned driver also measures actual two-second strategy ticks
separately. Its exact [four-metric source](data/four-metric-driver.c) is retained.
The final benchmark additionally measures faction tactics and the complete game
step, which includes ordinary enemies, projectiles and streaming. Movement-only
timings must not be presented as the whole combat cost. Most per-frame strategy calls do not run the strategic update, so
their percentile alone is not the update cost. The concurrent long-run maxima
include scheduler contention. Isolated measurements are labelled separately.

The browser timing build relinks the ordinary web objects and core with
[review-only wrappers](data/profile_browser.c). It is packaged and rendered in
Chromium, and is never uploaded as the game. The ordinary release is tested
separately. WebAudio capture observes the real output graph and checks finite,
non-silent, unclipped samples; that does not substitute for listening on physical
speakers or judging whether the cues are pleasant and understandable.

Human solo/split-screen readability, physical Windows graphics/controllers,
mobile GPUs and Safari remain unperformed. Use the
[checkpoint checklist](../../playtests/HIDDEN_FACTIONS_CHECKLIST.md) to record
those sessions independently of automated passes.
