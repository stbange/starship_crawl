# Spec Delta

## Purpose

Provide an optional standalone immersive VR version with tracked views and
controllers while preserving the ordinary desktop and browser game.

## ADDED Requirements

### Requirement: Independent VR target
The project SHALL provide an opt-in standalone Quest browser build with distinct artifacts and a separate public address. Ordinary Linux, Windows and web builds SHALL retain their default configuration, controls, rendering, storage and compatible game rules without requiring VR dependencies.

#### Scenario: Build ordinary targets
- **WHEN** the project is configured with its existing native or browser commands
- **THEN** it builds the ordinary game with no XR session code or XR launch controls.

#### Scenario: Build the Quest variant
- **WHEN** the optional Quest build is enabled and its named target is built
- **THEN** a separate immersive browser application is produced without overwriting ordinary artifacts.

### Requirement: Deliberate immersive entry
The VR launch page SHALL detect immersive support, start a session through an explicit user gesture and report unsupported browsers or rejected requests without starting uncontrolled gameplay. Exiting VR SHALL return to a paused usable launch page.

#### Scenario: Enter and exit
- **WHEN** the user enters VR and later ends the session
- **THEN** tracked play starts once and the expedition returns paused to the launch page with no stale held input.

#### Scenario: Unsupported headset session
- **WHEN** immersive sessions are unavailable or permission is rejected
- **THEN** the page explains the result and retains access to saves and a retry when appropriate.

### Requirement: Tracked stereo presentation
VR SHALL render one player through the headset's independent eye projections and poses, with stable metre-scale geometry, tracked looking and leaning, calibrated seated/standing height, and a single shared simulation update. Head tracking SHALL continue while menus pause gameplay.

#### Scenario: Look and lean
- **WHEN** the headset turns, tilts or moves within its supported tracking area
- **THEN** both eye views follow their actual poses with correct relative separation and without advancing the game twice.

#### Scenario: Inspect a paused menu
- **WHEN** the user opens an immersive menu and moves their head
- **THEN** the interface and world retain stereo tracking while the simulation clock stops.

### Requirement: Controller aiming and interactions
Tracked controllers SHALL support independent weapon aiming, a directional flashlight and ordinary interactions. Weapons SHALL preserve energy, cooldown, progression and hit rules; tracked origins SHALL be finite, bounded and prevented from shooting through intervening geometry.

#### Scenario: Aim away from the head
- **WHEN** the player looks one way and fires the held gun another way
- **THEN** visible shots or beams follow the controller ray while the view remains headset-controlled.

#### Scenario: Reach through a wall
- **WHEN** a tracked firing origin lies beyond nearby blocking geometry
- **THEN** it cannot bypass that geometry to damage targets.

### Requirement: VR locomotion and comfort
VR SHALL provide controller locomotion, deliberate snap turns, jump/crouch and vertical travel, recalibration and an optional movement vignette. Discontinuous changes such as respawn, shelter/matrix transitions and vehicle mounting SHALL receive a brief comfort fade. Unsafe head positions SHALL be obscured rather than reveal geometry through walls.

#### Scenario: Turn deliberately
- **WHEN** the turn stick crosses its threshold and remains held
- **THEN** it produces one snap turn until released through the neutral range.

#### Scenario: Change spaces
- **WHEN** an ordinary interaction transfers the player between physically separate spaces
- **THEN** a bounded fade covers the discontinuity and tracking resumes at the resulting position without changing progression.

### Requirement: Immersive game interface
VR SHALL provide a tracked wrist display and readable floating panels operated with controllers. The player SHALL be able to inspect navigation, select weapons, use available abilities, review and spend earned progression, make story choices, save, and access settings without a keyboard or a desktop-only overlay.

#### Scenario: Earn and use progression
- **WHEN** a VR player earns a guardian choice or character upgrade
- **THEN** controller-operated panels can select it through the ordinary progression rules.

#### Scenario: Review navigation
- **WHEN** the player checks the wrist display or opens the map
- **THEN** current survival information and ordinary mission/route guidance remain available in the immersive view.

### Requirement: Safe session lifecycle
Loss of usable tracking, required controllers, visibility or the graphics context SHALL stop gameplay and clear held actions. Recovery SHALL require fresh deliberate input and SHALL not replay elapsed wall time. Unrecoverable graphics loss SHALL provide a clear reload path.

#### Scenario: Remove a controller
- **WHEN** a required controller disappears while movement or fire is held
- **THEN** the simulation pauses, the action stops and reconnection cannot fire or move automatically.

#### Scenario: Resume after suspension
- **WHEN** a suspended session becomes visible and tracked again
- **THEN** it remains paused until deliberately resumed and does not simulate the suspended interval.

### Requirement: Independent compatible VR saves
VR SHALL use its own browser persistence and settings namespace, preserve existing save/generator versions, and support explicit compatible save import/export. Imports SHALL be transactional and SHALL not silently overwrite ordinary-web progress.

#### Scenario: Use both builds on the same origin
- **WHEN** the user saves different expeditions in ordinary web and Quest pages
- **THEN** each page resumes its own expedition and preferences independently.

#### Scenario: Transfer a save explicitly
- **WHEN** the user imports a supported save into VR
- **THEN** its earned character and world state load under the existing compatibility rules; invalid input leaves the current expedition intact.

### Requirement: Honest VR validation
The project SHALL test VR input/lifecycle and actual stereo WebGL output with external emulated devices, verify ordinary-target regressions, and publish a physical Quest checklist. Performance targets SHALL remain targets until measured on the named headset; software browser tests SHALL not be represented as headset playtests.

#### Scenario: Publish a VR build
- **WHEN** a Quest build is made available
- **THEN** its validation record distinguishes executed automated checks from pending headset frame-rate, comfort and controller checks.
