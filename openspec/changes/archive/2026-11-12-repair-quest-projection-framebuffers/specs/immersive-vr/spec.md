## ADDED Requirements

### Requirement: Compatible headset render targets
Quest SHALL validate each projection target before advancing gameplay and use
its runtime viewport inside the XR callback. It SHALL prefer granted projection
layers and retain base-layer compatibility otherwise, update simulation once
per frame, identify its rendering path in diagnostics and release owned graphics
storage on exit without deleting runtime images.

#### Scenario: Missing opaque attachments on a supported Quest
- **WHEN** the runtime's opaque base-layer target lacks attachments but granted projection layers provide valid view textures
- **THEN** both tracked eye views render through valid attached textures without using the incomplete opaque target.

#### Scenario: Omitted compositor depth
- **WHEN** a valid projection color texture has no accompanying depth texture
- **THEN** bounded reusable application depth storage preserves correct 3D depth rendering and is released on exit.

#### Scenario: Projection layers unavailable
- **WHEN** the browser lacks the projection API or does not grant the optional layer feature
- **THEN** the existing valid base-layer rendering and safe session lifecycle remain available.

#### Scenario: Invalid view image or session exit
- **WHEN** a required view texture is missing, its framebuffer is invalid, or the session ends
- **THEN** invalid rendering stops with usable recovery controls, owned graphics storage is released, and runtime textures are not deleted by the application.
