# Quest framebuffer startup investigation

The physical Quest 3 report from Oculus Browser 154.1 / Chromium 154 has one
valid head pose, no controllers, zero completed frames/eyes and
`Quest eye failed: GL 0x506 / renderer 0 / mesh 0`. `0x506` means
`GL_INVALID_FRAMEBUFFER_OPERATION`. The old report reads the GL error only after
an entire eye draw, so it cannot identify which operation generated it. Missing
controllers should produce a visible recovery panel and are not themselves a
framebuffer error.

The external test device can reproduce that exact error signature against the
pre-fix packaged application: queue a real GL error by clearing an incomplete
test framebuffer during `makeXRCompatible`, then supply only a valid head pose.
The old application exits on its first eye even though its current eye target is
complete. This demonstrates a sticky-error handling defect. It does **not** prove
that the same setup operation failed on the user's headset.

The repair freezes the SDL preview before asynchronous graphics setup. JavaScript
binds the runtime-owned opaque framebuffer inside the XR callback, C renders into
that binding, and JavaScript unbinds it before leaving the callback. The opaque
object no longer enters Emscripten's application-owned framebuffer table.

At the first usable headset callback, pending GL errors are retained in the
report. Only a pre-existing `0x506` permits an attempted recovery: the actual XR
target must be complete and new preparation, clear and draw operations must all
succeed. Other setup errors, incomplete targets and any new render error still
stop the session, save/pause the expedition and return to the launch page.
Diagnostics now identify the graphics stage and framebuffer status. The build
identifier is `quest-framebuffer-recovery-2`.

Extra GL error queries distinguish preparation/clear/draw during the first
stereo frame. Subsequent frames keep the existing one end-of-eye error query,
avoiding additional steady-state CPU/GPU synchronization on the headset.

Quest packaging gives both JS and WASM requests the same content-derived cache
revision. Changing either artifact changes both request URLs, so a page reload
cannot combine the new bridge with a cached older C export signature. The build
outputs and ordinary packager are unchanged.

The emulator now enforces the WebXR rule that an opaque framebuffer is incomplete
outside its session's animation callback. Fault injection lives entirely in
`tests/quest_device.js`; no graphics or game testing controls ship with the game.
The regression covers both controllers missing, stereo recovery imagery,
controller return, deliberate A resume, session re-entry, a genuinely incomplete
first target, draw failure, clear failure after successful entry, and a different
setup GL error that must remain fatal.

Only the opt-in Quest application and its browser tests change. Core gameplay,
generator/save formats and ordinary-target settings are unchanged. Rebuilding
ordinary web produces byte-identical HTML, JavaScript and WASM to the pre-fix
package. Physical Quest startup, performance and comfort remain unverified here;
the updated report is needed if the headset still fails.

The retained evidence includes the [pre-fix failure](before.txt),
[recovered report](recovered-report.json), [stereo capture](recovery-stereo.png),
[genuine failure reports](failure-reports.json), and [validation record](verification.json).
Local checks passed: 23 specification items, 31 native unit/platform tests,
seven Quest unit tests, solo exterior and split-screen hall render smoke checks,
all 13 full Quest browser check groups, and three ordinary browser suites.
After the final diagnostic/performance refinements, the versioned Quest package
passed the focused startup suite again. Package tests also verify coherent asset
versions and protection of the ordinary site. Full CI remains the publication gate.
