# Deep-expedition checkpoint gallery

All images are actual Chromium/WebGL renders of saved expedition states, imported using the game's file UI. Nothing is generated artwork or an unrelated inspection scene. Each JSON sidecar records the rendered state plus the native checkpoint check (seed, x, y, z, coordinate layer, floor-supported, room-ready, failed). A load can finish/normalize a lift transit, so recorded screenshot coordinates can differ from a moving character's telemetry. Exposure and HUD are unmodified.

Every route has three views: `minute-15`, `minute-45` and `deepest`. The deepest checkpoint can precede the hour's end. All start on seed 713 and show earned characters.

| Route | 15 minutes | 45 minutes | Deepest checkpoint |
| --- | --- | --- | --- |
| Inward | [View](00-inward-minute-15.png) | [View](00-inward-minute-45.png) | [View](00-inward-deepest.png) |
| East | [View](01-east-minute-15.png) | [View](01-east-minute-45.png) | [View](01-east-deepest.png) |
| West | [View](02-west-minute-15.png) | [View](02-west-minute-45.png) | [View](02-west-deepest.png) |
| Up | [View](03-up-minute-15.png) | [View](03-up-minute-45.png) | [View](03-up-deepest.png) |
| Down | [View](04-down-minute-15.png) | [View](04-down-minute-45.png) | [View](04-down-deepest.png) |
| East + inward | [View](05-east-inward-minute-15.png) | [View](05-east-inward-minute-45.png) | [View](05-east-inward-deepest.png) |
| West + inward | [View](06-west-inward-minute-15.png) | [View](06-west-inward-minute-45.png) | [View](06-west-inward-deepest.png) |
| Up + inward | [View](07-up-inward-minute-15.png) | [View](07-up-inward-minute-45.png) | [View](07-up-inward-deepest.png) |
| Down + inward | [View](08-down-inward-minute-15.png) | [View](08-down-inward-minute-45.png) | [View](08-down-inward-deepest.png) |
| East + down + inward | [View](09-east-down-inward-minute-15.png) | [View](09-east-down-inward-minute-45.png) | [View](09-east-down-inward-deepest.png) |

To replay in another workspace, compile `../data/check-checkpoint.c` with the native core into `/tmp/check-deep`, package the browser game using the standard project instructions, and run `data/replay.py` from the repository root. Set `SC_REVIEW_SITE` to the parent of the packaged `starship_crawl` folder and `SC_BROWSER` to your Chromium path. The review used the existing `build-pages-tentacles-final` package built from the reviewed game commit.

## Looking up inside rooms reached along the routes

- [East/inward branching concourse at 45 minutes](05-east-inward-minute-45-upward.png).
- [West/inward hanging-garden footprint at 45 minutes](06-west-inward-minute-45-upward.png).

These additional views use ordinary mouse movement in the same imported locations. They complement the 30 consistent eye-level checkpoints and retain their own recorded-state sidecars.
