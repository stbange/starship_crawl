## Context

The reported `0x8cd7` indicates missing attachments rather than a missing pose.
Ignoring it would risk another black frame and a genuine GL draw failure.

## Decisions

Request `layers` as an optional feature. When granted and implemented, use
`XRWebGLBinding.createProjectionLayer` with ordinary 2D textures. Obtain the
current view's color/depth images during its XR callback, attach them to an
application-owned framebuffer, and use its runtime viewport. Allocate a reusable
depth renderbuffer only when no depth image is supplied. Preserve the existing
base-layer implementation for browsers without this capability.

Validate attachments before advancing the frame, detach runtime textures before
leaving the callback, and delete only application-owned framebuffer/depth storage.
Preserve one simulation update per headset frame and existing deliberate recovery.
The external device must model the failed opaque target and projection textures
independently; it must not grant gameplay state or ship with the application.

## Validation and Limits

Test stereo output, replaced runtime images, absent depth, missing controllers,
denied/absent layer capability, target failure, exit/re-entry and resource ownership.
Run the full Quest browser regression and packaging/isolation checks. No physical
headset is available here; automated results cannot confirm headset performance
or the device-specific repair until the user retries the published build.
