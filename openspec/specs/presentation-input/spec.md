## Purpose

Retro 3D presentation and stable input across desktop, web, touch, and optional split-screen.

## Requirements

### Requirement: Retro atmosphere

The renderer SHALL provide low-resolution pixelated 3D, localized broken colored lamps, deterministic flicker, infestation, and a per-player flashlight.

#### Scenario: Flashlight controls

- **WHEN** a player uses keyboard, gamepad, browser menu, or touch lamp control
- **THEN** that player's flashlight toggles and other local players retain their own state.

### Requirement: Stable capture and aiming

Boarding and resuming SHALL preserve camera orientation across pointer capture, and actual locked relative mouse movement SHALL control web aiming.

#### Scenario: Boarding

- **WHEN** the cursor is positioned over a menu before entering gameplay
- **THEN** capture and menu motion do not turn the camera.

#### Scenario: Mouse aim

- **WHEN** the captured pointer delivers relative motion
- **THEN** the first-person view responds to deliberate motion without duplicate SDL motion.

### Requirement: Portable controls

The game SHALL support mouse/keyboard, gamepad, simultaneous movement and fire touches, and resize/orientation changes.

#### Scenario: Mobile multitouch

- **WHEN** a player holds movement and fire with separate fingers
- **THEN** movement and shooting operate together and releasing fingers stops those actions.

### Requirement: Optional multiplayer

Play SHALL start solo and add or remove a second local player only through an explicit action.

#### Scenario: Independent distant views

- **WHEN** players occupy distant coordinates in split-screen
- **THEN** each camera renders its own neighborhood with stable coordinates and bounded shared streaming.

### Requirement: Persistent control preferences
Players SHALL be able to adjust mouse, touch and gamepad look sensitivity, gamepad deadzone and look inversion through native and browser controls. Invalid preferences SHALL fall back to bounded safe values. Preferences SHALL survive a restart and remain independent of expedition imports and new ships.

#### Scenario: Tune and restart
- **WHEN** a player changes sensitivity or inversion and restarts
- **THEN** the selected settings remain active without modifying the character or ship.

### Requirement: Per-view rendering preferences
Each local view SHALL support selectable internal render resolution while preserving a readable independently rendered HUD. Split-screen and mobile views SHALL have practical defaults and bounded render target sizes.

#### Scenario: Tune a split view
- **WHEN** one player's render quality changes
- **THEN** that view's pixel density changes while the other view retains its setting and both HUDs remain legible.

### Requirement: Stable input ownership
Controller disconnects SHALL pause safely, and a remaining controller SHALL retain its assigned player. Reconnecting or explicitly joining SHALL restore a usable mapping without silently taking over another player's character.

#### Scenario: Remove a controller in co-op
- **WHEN** one of two connected controllers disconnects
- **THEN** the game pauses, held input clears, and the remaining controller continues to address the same character after resuming.

### Requirement: Input lifecycle reset
Focus loss, backgrounding, touch cancellation and orientation changes SHALL clear stale held movement, fire and look input. Resuming SHALL not replay elapsed simulation time or accumulated pointer motion.

#### Scenario: Rotate during a gesture
- **WHEN** the viewport changes orientation while a touch gesture is active
- **THEN** the old gesture stops and fresh movement/look/fire touches work in the new layout.

### Requirement: Honest platform validation
Platform reports SHALL distinguish compilation, automated runtime execution, virtual input, browser emulation and physical-device playtesting. Unperformed physical Windows graphics, gamepad, Safari and mobile checks SHALL have concrete reproducible instructions and remain explicitly unverified.

#### Scenario: Review validation results
- **WHEN** a build or milestone is reported as validated
- **THEN** the report identifies executed automated checks and any remaining physical-device checks separately.

### Requirement: Environmental sprite detail
Generated rooms SHALL support bounded depth-tested decorative sprites, steam, shallow water, colored rain, strange glows and local fog, selected by their environmental conditions.

#### Scenario: Explore damaged rooms
- **WHEN** a player views a leaking, venting or infested room
- **THEN** animated effects and anchored details express that condition without obscuring the HUD or removing combat telegraphs.

### Requirement: Independent environmental views
Atmosphere SHALL use stable world anchors, bounded per-view work and independent camera settings in optional split-screen. It SHALL not leak into the shelter, matrix or space.

#### Scenario: Separated co-op
- **WHEN** two players view different conditions or one enters the shelter
- **THEN** each view retains its appropriate fog and effects within the shared resource limits.

### Requirement: Living tentacle infestation
The renderer SHALL add tapered, curling animated tentacles with readable surface detail to seeded floor growths and wall nests, keeping the loading bay and low conduits clear. Floating gazers SHALL carry trailing tentacles. Environmental tentacles SHALL remain decorative and preserve existing routes, enemy capabilities and saved expeditions.

#### Scenario: Inspect an infestation
- **WHEN** a player approaches a generated growth or nest with the flashlight
- **THEN** rooted tentacles, suckers and faint luminous tips are visible and their movement stops when the game pauses.

#### Scenario: Revisit an existing ship
- **WHEN** an existing expedition is loaded or a room is streamed again
- **THEN** tentacles reconstruct from its existing growth anchors without changing architecture, collision or encounter identities.

### Requirement: Bounded tentacle views
Environmental tentacles SHALL use a bounded per-camera detail budget, favor nearby growths and retain stable world anchors. Tentacles from the ship SHALL not appear in sheltered, matrix or exterior views.

#### Scenario: Separate local players
- **WHEN** one player enters the shelter while the other observes an infestation
- **THEN** the first view contains no ship tentacles and the second continues to animate them within its own budget.
