# Expedition design review — 6 November 2026

Review of game commit `537f785d8b0ecd43aab08b0cab896975fb9c1f9c`. These are proposals for discussion, not implemented balance changes.

## Method and evidence

Three fresh single-player expeditions ran for **25 minutes of simulated gameplay each**, 90,000 fixed 1/60-second updates per seed. This is **75 simulated minutes, not 75 minutes of human or real-time rendered play**. The headless simulation ran faster than real time (approximately 11.17, 12.03 and 15.36 wall seconds for seeds 713, 42 and 3). Native core and the packaged WebGL game were used; no physical hardware playtest was performed.

The driver adapts the existing campaign journey bot. It moves, aims, fires, interacts, chooses guardian rewards and buys upgrades through ordinary core APIs. It does not teleport, remove enemies, inject damage or grant equipment. It reads generated topology and has perfect aim, so its route and combat performance are optimistic. It always uses the starter carbine, follows one guardian branch, takes a deterministic reward option and aggressively buys power/vitality/armor upgrades. It does not test the full range of weapons, vehicles, psionics, hacking or shelter strategy. Closing journals is automated UI state. One normal save/reload occurs after phase two, inherited from the original journey driver.

Raw 30-second samples and event logs, checkpoint saves, the exact adapted driver and a machine-readable summary are under [data](data). Sampling cannot measure every damage event, short stall or brief room visit. Screenshots are **re-rendered checkpoints imported through the browser's actual file UI**, with seed and rank checked. They are not continuous video: save/load reconstructs populations and resets active guardian attempts according to the existing game rules. Their JSON sidecars record the rendered state. Early native smoke attempts were discarded because smoke mode intentionally bypasses save loading.

## Results

| Seed | Duration | Phase four reached | Final kills | Deaths | Final rank | Distinct themes sampled | Conditions sampled |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 713 | 25:00 | 7:18 | 663 | 0 | 34 | 16 | 6 |
| 42 | 25:00 | 6:01 | 774 | 0 | 37 | 19 | 6 |
| 3 | 25:00 | 6:29 | 213 | 0 | 19 | 9 | 5 |

The foothold guardian fell at 18.1–18.5 seconds and phase one was reached at 59.7–61.2 seconds. These timings exclude human orientation and deliberation. Phase four is **not** proof that the story extraction mission finished.

Seed 3 is the most important exception: from the 12:00 sample through 25:00, the bot remained at approximately `(-66.09, -186.30, 48.00)`, with kills and shots unchanged. Its displayed goal was `[0,-1,4]`. Thus this run includes roughly 13 minutes of a navigation stall, rather than 25 minutes of successful exploration. The center-to-center driver is too simple to distinguish a bad waypoint from an inaccessible route; this is a reproducible investigation target, not a proven world-generation softlock. The late checkpoint and browser recovery probe are retained. Ordinary strafe/jump/forward input moved the character from `(-66.09, -186.30, 48.00)` to `(-69.90, -208.60, 54.75)`, so the character is not immobile. This supports investigating the bot/landing route first; it does not establish that the intended objective is reachable.

## Design assessment and proposed morning backlog

### 1. Investigate vertical navigation before increasing difficulty

**Observed:** one seed stalls for the final half of its run while the others continue across decks. Its height lies between deck floors. The topology-aware driver still cannot reliably handle furnished routes and vertical geometry.

**Proposal:** reproduce seed 3 from its late checkpoint with manual controls, inspect the actual stair/ramp/lift connection and compare intended deck versus current floor support. Give the objective map an explicit next vertical connection and mark the destination deck. Add short local breadcrumb cues at lift landings and a recover-to-last-safe-landing action if the player remains unsupported. Make the automated driver replan after several seconds without progress and use actual portal/landing waypoints rather than room centers.

**Acceptance:** walk the suspect connection both ways; no coordinate injection or invulnerability. Repeat on multiple seeds with a bounded no-progress detector. Do not declare the generator fixed merely because the bot takes a different route.

### 2. Make rewards change the way a player fights

**Observed:** a driver that deliberately refuses to switch away from the first gun can clear all four phases on all three seeds. The final weapon inventories differ (masks 16395, 262403 and 75), but the behavior remains identical. This shows starter-gun viability; it does not prove that alternatives are unfun or weak.

**Proposal:** keep the reliable starter gun and add encounters that reward earned tools: shield links vulnerable to sustained beams, clustered brood that rewards area damage, elevated nests that encourage flight, and heavy suppression that makes a mech attractive. Introduce a safe demonstration immediately after earning a capability, followed by an optional encounter where using it is clearly advantageous. Avoid mandatory single-weapon locks.

**Acceptance:** repeat equivalent routes with several builds and imperfect aiming. Compare movement choices, target priority, encounter duration and damage received. Each reward should produce a noticeable new decision within the next two encounters.

### 3. Add pressure through enemy coordination before inflating health

**Observed:** zero deaths in the idealized runs; 30-second health samples never fall below 100 and energy samples are always 100. Sampling misses transient damage, and upgrades/mission recovery affect these values. Source inspection confirms regenerative defenses and a substantial energy refill rate. This is insufficient evidence for a blanket damage or regeneration nerf.

**Proposal:** use coordinated roles: a visible anchor organism supports a few attackers, one flanker takes a side route, and a ranged enemy commits to a dodgeable shot. Limit simultaneous attack commitments so the player can read the threat. A destroyed support should visibly change the group. Keep fodder quick to kill and reserve durability for a readable armored part or an exposed core.

**Acceptance:** maintain fast horde clearing while making the player move and prioritize. Measure attack overlaps and warning-to-impact times. Test ordinary aim and reaction delays before tuning numerical difficulty.

### 4. Shape a deliberate tension-and-release rhythm

**Observed:** all phase transitions happen early with the informed bot. Seeds vary substantially in sampled content, but room variety alone cannot establish whether the adventure has a satisfying dramatic arc.

**Proposal:** compose each phase from an arrival/view, an environmental trace, a short fight, an optional detour, a more distinctive encounter, a quiet staging area and a guardian. Let district changes be legible before the next swarm. Use the enormous rooms for distant movement and silhouettes, then concentrate combat around a few readable landmarks. Follow narrow passages with a reveal of scale. Let the shelter offer a breathing space without becoming required after every room.

**Acceptance:** in human first-play runs, record time spent lost, travelling, fighting, choosing rewards and reading. Test a provisional 20–30 minute full expedition target rather than lengthening every corridor to compensate for the bot's speed.

### 5. Give small stories a consequence

**Observed:** the route samples many room themes and conditions. The bot does not read the dossier, so these runs cannot evaluate narrative engagement.

**Proposal:** make a visible local scene invite one small action: close a failed containment valve, follow wet evacuation tracks, recover a technician's tool, or choose whether to energize a botanical ward. Each should change something nearby: light, enemy behavior, a shortcut or a small reward. Tie its evidence to the existing local account and main ship motivation. Avoid filling the map with generic objectives.

**Acceptance:** a player should infer what happened from the room before opening F6, and notice the consequence without reading a completion popup.

### Visual readability from checkpoint replays

The captured rooms retain a strong dark palette and striking cyan/amber structural accents, especially the Turbine Gallery and Crew Sepulchres. At ordinary eye height, several different themes read first as a dark rectangular opening, while a large amount of text competes at the top of the view. This small screenshot sample cannot judge the entire generator. I would test one unmistakable silhouette per important room, a dim distant light that reveals its extent, and a simpler HUD that promotes the current actionable direction and moves secondary information into F6/F10. Preserve darkness between focal points rather than raising global brightness.

## More alien, strange and disturbing enemies

The current core already supports climbing, hopping, charging, hovering, cloaking, healing, brood spawning and ranged windups. Many creatures still choose a player-facing direction and use periodic movement. The next improvement should be **recognizable individual rules and unusual motion**, with clear counterplay, rather than another batch of recolored stat variants.

| Proposal | Behavior and animation | Where it belongs | Fair warning and response |
| --- | --- | --- | --- |
| Backwards pilgrim | A husk travels backwards while its head and rib cage independently track the player; it folds forward only to strike. | Shrines, crew processions | Shoulder twitch and rising breath precede a committed, dodgeable lunge. |
| Listening membrane | A hanging organism turns toward footsteps and shots, then sends a ripple through nearby dormant growth. Its surface puckers where sound arrives. | Quiet service passages, archives | Visible ripple and audible clicks reveal its range; walk quietly or destroy the membrane first. |
| Gaze-bound scavenger | It slowly unfolds while unobserved, then holds an anatomically impossible pose when watched. | Long galleries and luxury spaces | Reflective eyes and a scrape reveal relocation; it moves through supported space, never teleports into melee. In co-op, observation by either player counts. |
| Ceiling seam-runner | Limbs plant before the torso follows; the body rotates late when crossing from wall to ceiling, with a trailing abdomen. | Industrial ribs, vault edges | Dust or drips fall before a drop attack; the landing mark stays visible and the leap commits to a location. |
| Reversing worm | It can change which end is the head: segment contraction travels one way, then reverses, and a feeding ring opens at the former tail. | Runoff channels and gardens | The newly active mouth lights before attack; both ends remain hittable and the turn has a recovery window. |
| Colony in a coat | A humanoid silhouette separates into three linked pieces around cover, then reforms elsewhere along a visible tether. | Infected crew areas | The tether is the vulnerable target. Killing the core ends the whole encounter and grants rewards once. |
| Molting sentinel | Armor loosens and drops as an empty shell; a thinner, faster inner creature emerges with a different gait. | Research and quarantine | An obvious split, change in silhouette and brief vulnerable transition; at most one molt, no fake loot or repeated resurrection. |
| Pendulum gazer | The eye remains unnervingly level while the body and tentacles swing around it; the pupils contract before firing. | Upper galleries and condensate zones | Its shot uses a visible aiming lock and a fixed release moment; sideways movement defeats the lock. |
| Nest conductor | Several small creatures stop together, orient toward a hidden organizer, then attack in staggered pulses. | Dense bloom, containment wards | A luminous link identifies the conductor; killing it breaks coordination and briefly scatters the pack. |
| Wrong-foot machine | A robot pauses mid-step, rotates only its torso through an excessive angle, then takes several very precise steps with no idle sway. | Command and machine decks | Hydraulic clicks count down to a committed sweep; exposed joints provide a clear counter. |
| Doorframe ambusher | A thin creature rests inside a real recess, unfurls its arms across the doorway, then retracts if missed. | Tight crew thresholds | A moving seam and bioluminescent nodules identify it before entry. Never seal the only exit or spawn on top of a player. |
| False reflection | A creature reproduces a distorted silhouette on nearby wet surfaces while its physical body stays elsewhere in the room. | Baths and flooded luxury areas | Ripples reveal the mismatch; the body remains reachable and the illusion never fakes HUD, controls or save state. |

### Recommended first implementation

Start with **backwards pilgrims, pendulum gazers and seam-runners**. They reuse existing body families and terrain normals, have strong silhouettes, and can be judged in first-person quickly. Add the **nest conductor** next to make ecology affect actual encounter decisions. Save colony splitting and reflection tricks for later: they need more encounter-state and rendering work.

For each family, author five states: dormant/ambient, alerted, approach, attack commitment and recovery/death. Give each a distinct timing signature. Use delayed torso motion, contact-anchored limbs, asymmetric breathing and short interrupted movements. Make motion depend on speed and attack state so creatures do not treadmill in place. Reserve occasional stillness for contrast with the fast swarms.

Disturbance should come from violated expectations, not lost player control. Keep hitboxes tied to readable bodies, expose attack preparation through both sound and movement, retain projectile visibility in fog, and give every frightening action an escape or interruption opportunity. Do not shake or steer the player's camera, erase interface information, add unavoidable grabs or resurrect already rewarded enemies. In split-screen, agree on one world state even when the two players observe different things.

### Evaluation targets

- Can a player identify the family from silhouette and motion before reading its name?
- Can a first-time player explain what warned them before a damaging attack?
- Does the new behavior change positioning or target priority instead of merely slowing kills?
- Does it fit its district, local story and available geometry?
- Do pauses, deaths, save/load and separated co-op preserve a consistent state?
- Does a crowd remain readable and within current simulation/rendering budgets?

## Suggested morning discussion

First review the seed-3 navigation evidence. Then choose whether the next milestone is the three reusable alien animation families plus one coordinated nest encounter. Tune phase pacing and power only after a human first-play session and an imperfect-input bot comparison. Preserve the game's central promise: eerie exploration and strange inhabitants, with generous, powerful horde combat.

## Reproduction

Build the existing native core, then compile the retained diagnostic driver from the repository root:

```sh
cc -O2 -Isrc docs/reviews/2026-11-06-expeditions/data/review-driver.c build/libsc_core.a -lm -o /tmp/sc-review-driver
mkdir -p /tmp/sc-review-713
/tmp/sc-review-driver 713 /tmp/sc-review-713
```

Repeat with seeds 42 and 3. The driver writes `phase-minute-00/05/10/15/20/24.sav`; the last name uses integer division of the final frame index, but its recorded time is 1500 seconds (25 minutes). Use browser Import to inspect a checkpoint; native `--smoke` intentionally does not load saves. See the [screenshot gallery](screenshots/README.md).
