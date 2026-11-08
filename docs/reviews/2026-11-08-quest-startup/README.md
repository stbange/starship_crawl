# Quest startup recovery

A user reported a black view and unresponsive controllers on a physical Quest 3.
Its browser version, entry timing and system-menu behaviour were not available
for this repair. The exact cause on that headset is therefore not confirmed.

A packaged pre-fix build from the released code reproduces a matching failure
with the standard-API test device: start with a valid head pose and one missing
controller. After 40 XR callbacks the game reports an active, suspended session
but **zero rendered frames and zero eye views**, and the actual framebuffer is
completely black (mean RGB 0, 0, 0). The launch page incorrectly says VR is active.
The initial emulator always supplied all controller poses, missing this case.

The adapter now retains independently valid poses and can calibrate/render from
the head alone. Missing or invalid hand poses still pause gameplay and prevent
firing/movement. A large head-following panel identifies the waiting controller;
returning controllers require neutral controls followed by a fresh A press.
Vertical head orientations at entry use the horizontal right axis for calibration.
Unsafe leaning has a separate visible recenter instruction while world geometry
remains obscured. These changes affect only the opt-in Quest target.

The bridge reports actual startup progress and bounds failed entry: 12 seconds
without XR callbacks or 20 seconds without usable tracking while visible return
to the launch page. These timers stop after successful tracked rendering; later
controller recovery stays paused until deliberate resume. A first full regression
run caught an overly broad timeout during a software-rendered wrist capture; the
final implementation scopes that timeout to initial entry. Render errors and the
launch page's copyable troubleshooting report retain failure stage, controller
mapping/profiles, pose and frame counts, framebuffer dimensions and recent logs.
The report stays local; no automatic diagnostic upload is added.

## Captures and validation

[Before: black framebuffer](before-black.png) ·
[Controller recovery](recovery-panel.png) ·
[Recovery while looking around](recovery-turned.png) ·
[Lean/recenter guidance](recenter-panel.png)

These are real WebGL framebuffer captures from the external software headset,
not photographs from the user's Quest. The [verification record](verification.json) retains all ten passing browser
check groups, 44 passing native tests, five passing Quest sanitizer checks,
98.0% core unit coverage (10,258/10,470 lines; every file at least 95%),
100% tracking-adapter coverage and unchanged ordinary artifact hashes.
The [example report](example-diagnostics.json) comes from the deliberately
unsupported-controller-mapping case. The complete publication gates still run
on every main push, including Linux/Windows, all 18 ordinary browser suites,
Quest, unit-only coverage, specifications and instrumented campaign tests.

The ordinary packaged HTML, JS and WASM are byte-identical to the previous
ordinary package. Shared gameplay, save format and generator versions are
unchanged. Physical headset startup, sustained performance and comfort still
require confirmation using the updated Quest page and hardware checklist.
