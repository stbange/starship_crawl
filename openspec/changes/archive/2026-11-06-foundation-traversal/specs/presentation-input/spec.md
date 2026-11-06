## ADDED Requirements

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
