## Rules and ownership
The ship seed and generator determine geography, traits, guardian sites, families and reward offers. A bounded campaign state stores completed encounters and each character's claimed rewards, powers, rigs, modules and equipped loadout. World access derives from completed phases. Existing ships retain their original rules. New-ship creation resets both player profiles, all matrix progress and all temporary effects.

## Geography
Four nested bands use whole four-cell regions and deck pairs so landmarks never straddle a gate. Each phase reserves two large halls on opposite approaches. A first-band optional elite supplies an early capability. Every region remains connected, all routes obey the same physical access boundary, and completed phase access applies to both players. Thermal and order axes/polarities are seeded and stable; sector function and historical sites remain recognizable.

## Combat and rewards
Guardian bodies reuse existing enemy silhouettes with dedicated telegraphed patterns and vulnerable periods. Their encounters start explicitly at staging terminals, lock scaling to participants, and reset when all participants withdraw or die. Completed guardians stay defeated. A reload during an unfinished attempt returns its participants to staging and resets the attempt. Rewards are deterministic three-way offers with useful duplicate compensation, separate receipts per eligible player and no reward from merely loading a room. Two gun slots, two power slots, one rig and one specialization are changed at the shelter or safe staging.

## Integration
Generated story targets belong to deeper bands and require the final boundary victory. Power restoration and security controls reduce optional guardian hazards without providing an instant win. The phase journal provides route/reward previews, pending choices and refitting; ordinary mission and matrix journals remain available. Completed main objectives still lead to the existing generated conclusion and optional same-ship continuation.

## Compatibility and validation
Append a small explicitly serialized campaign block to format 10. Older files initialize legacy campaign state, never lose existing profiles on resume, and start fresh only on an explicit new-ship action. Validate all masks, slots, receipt ownership, phase ordering, coordinates and completed-room consistency transactionally. Test original rules separately from new-game campaigns; retain legacy fixtures and native/WASM semantic parity. Pacing and enjoyment require subsequent user playtesting.
