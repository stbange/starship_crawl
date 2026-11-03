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
