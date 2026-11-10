# Spec Delta

## ADDED Requirements

### Requirement: Transformative specialization branches
Coherent-expedition specializations SHALL offer mutually exclusive branches with different
observable combat behavior. Only one specialization branch SHALL operate at a
time, within the existing loadout limits. Safe refitting SHALL explain the effect,
requirements and tradeoff before confirmation. Locked tools SHALL remain locked.

#### Scenario: Choose a beam relay branch
- **WHEN** an eligible character equips the relay branch and fires a compatible beam through an attached flare
- **THEN** the declared bounded relay effect occurs, while an unequipped alternative branch contributes no effects.

#### Scenario: Preview an unusable upgrade
- **WHEN** a character inspects a branch requiring an unearned power or rig
- **THEN** its requirement is clear and inspection neither unlocks that tool nor spends resources.

### Requirement: Bounded growth and useful rewards
Coherent-expedition growth SHALL preserve increasing power, bounded resources
and forgiving checkpoint death. Purchasing an attribute SHALL NOT provide an
undeclared full-resource combat reset. Challenge rewards SHALL preview usable
choices or explicit useful alternatives for owned items, with persistent
per-participant claims and no duplicate payment.

#### Scenario: Upgrade while injured
- **WHEN** an injured character purchases an attribute
- **THEN** only its declared capacity or resource effect occurs, with health, armor, shield and energy remaining within their new bounds.

#### Scenario: Own every offered item
- **WHEN** a participant qualifies for a challenge whose item choices are already owned
- **THEN** the preview offers a defined upgrade or preparation resource with an available use, and the participant can claim one reward once.

#### Scenario: Earn phase growth with a temporarily absent partner
- **WHEN** a phase guardian is defeated under the coherent rules and a participating character has temporarily left the local view
- **THEN** each participant's stored profile receives four attribute points once, capped at 1,000, while the separate tool choice remains claimable later; the opening elite and archived rules retain their existing awards.
