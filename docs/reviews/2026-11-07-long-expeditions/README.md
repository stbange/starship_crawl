# Long expedition review — 7 November 2026

**Starship Crawl now has substantial exploration variety. Its strongest next
iteration is combat, reward and faction tuning: the character becomes dominant
quickly, while many encounters end before their distinctive mechanics matter.**
Keep the pleasure of clearing hordes and the forgiving death system. Build
tension through readable threats, consequential choices and worthwhile optional
risks.

Completed **13 × 60 simulated minutes**, using the production C gameplay core at
60 Hz: twelve fresh-character expeditions across twelve seeds, followed by one
controlled navigation comparison. That is **13 hours / 2,808,000 simulation
updates**, accelerated on the CPU, not thirteen hours of human play. The primary
twelve runs produced **10,910 kills and zero deaths**; the comparison adds 1,291
kills and no deaths. All reached phase four, completed primary extraction and
settled one political dispute. One primary run failed to progress deeply because
of its controller; its complete result remains in the dataset.

Reviewed game commit: [`2bcba27`](https://github.com/stbange/starship_crawl/commit/2bcba27396768744b3ffafcc4f4c86a404f091e1),
generator 13, save format 14. This work changes review tools and documentation;
it does not change the game or its saves.

## Runs and coverage

| Run | Seed | Direction after settlement | Input/build profile | Maximum layer | Room owners entered¹ | Kills | Lowest health² |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: |
| 01 | 713 | Inward | Offense | 40 | 282 | 1,243 | 100% |
| 02 | 42 | East | Survival | 31 | 201 | 791 | 70% |
| 03 | 1 | West | Toolbox | 32 | 227 | 928 | 78% |
| 04 | 3 | Up, touring each deck | Offense | 9 | 195 | 897 | 100% |
| 05 | 4 | Down, touring each deck | Survival | 7 | 151 | 646 | 92% |
| 06 | 14 | East/inward | Toolbox | 28 | 294 | 945 | 94% |
| 07 | 22 | West/inward | Offense | 31 | 324 | 1,378 | 100% |
| 08 | 109 | Up/inward | Survival | 28 | 298 | 869 | 95% |
| 09 | 314 | Down/inward | Toolbox | 26 | 309 | 1,166 | 68% |
| 10 | 2026 | East/down/inward | Offense | 4 | 95 | 452 | 94% |
| 11 | 9001 | West/up/inward | Survival | 18 | 246 | 623 | 87% |
| 12 | 8675309 | Alternating east/west, inward | Toolbox | 17 | 253 | 972 | 100% |
| 13 | 2026 | Repeat of run 10; revised recovery | Offense | 25 | 330 | 1,291 | 94% |

¹ Distinct generated owner coordinates entered, deduplicating cells belonging to
the same room. These are **not fully explored or cleared rooms**. Vertical tours
walk across decks; they are not comparable to the earlier lift-only depth runs.
Layer is the game's difficulty-layer calculation, not the deck number.

² Minimum observed health divided by the character's current maximum, sampled
every update. Damage can be absorbed by shields or armor without reducing health.
Per-update net losses can also hide damage healed within the same update. They
are conservative observations, not a complete incoming-damage event ledger.

The primary sample covers all four ship identities, all four motivations and all
four history causes. Each run enters 18–28 room themes; the combined sample enters
all 28. The room-entry population observations include 40 of 42 species; thirteen
personal weapons fire at least once. There are **118 deliberately claimed
optional battles**, **49 completed local actions**, and **184 claimed director
dispatches**. All twelve settle a country and return to its changed court; the
three settlement choices each receive four runs. Toolbox profiles use earned
flight/nova/rig equipment when applicable, with approximately 47–92 seconds of
active tool use per run. That is limited coverage of tool strategy.

The [data summary](data/summary.json), [aggregate](data/aggregate.json) and
[pacing plot](pacing.png) retain the numbers behind this review. Unless stated
otherwise, aggregate findings below refer to the primary twelve runs; the
thirteenth is a separately retained comparison.

## Findings that should shape the next implementation

### 1. Character growth overtakes the challenge

Seven of twelve characters buy **all 120 available attribute increments** within
the hour, first doing so between minutes **37.7 and 57.7**. They converge on the
same fully upgraded stat sheet despite different starting priorities. Rank,
salvage and spare points then continue accumulating. One final screenshot shows
19 unused points and more than 15,000 salvage.

Nine runs record **no further health loss during minutes 30–60**. The lowest
health ratio in the whole sample is 67.8%; there are no deaths. Shield losses
show that enemies do connect, so this is more than a zero-damage checkpoint
illusion. Buying *any* attribute restores health, armor, shield and energy;
level-ups also refill them. Immediate spending is therefore a healing strategy
as well as a build choice. The recorded upgrade/level defense increases include
capacity increases and must not be mistaken for pure healing totals.

Weapon energy rarely demands a response: only **3.62 seconds** in the twelve
hours are observed with insufficient energy for the selected personal weapon.
This measure excludes a separate assessment of mech weapon costs. The free
carbine remains highly effective; screenshots show its damage rising from the
base 27 to 140.

**Design implication:** tune the rate and shape of growth before adding another
large content tier. Preserve weak enemies that die satisfyingly. Put pressure
into elite combinations, short exposure windows and occasional costly decisions.
Test a smaller number of meaningful build commitments, partial recovery on
upgrade, and clear reasons to visit the existing shelter. Avoid making every
ordinary monster a damage sponge.

### 2. Royal guards rarely feel like a climax

Royal-guard active fights last **2.12–11.28 seconds, median 5.08 seconds**.
Eleven of twelve cause **zero observed combined health/shield/armor loss**; the
remaining fight causes 40. The full accepted-commission-to-settlement journeys
take 84–193 seconds. Across those journeys, both political battles together
occupy only about **3–10%** of the time. Travel dominates the political arc even
with instant automated dialogue choices.

The four phase guardians perform better. Their final-phase fights last roughly
9–29 seconds; one earlier guardian reaches 103 seconds on the slower profile.
This variation argues for testing each guardian/build pairing instead of applying
one health multiplier everywhere.

**Design implication:** give royal finales two or three clearly readable events
that survive the player's opening burst. The divided monarch could transfer a
ward between bodies; the molting ruler could leave a dangerous shell; the echoes
could repeat a previously telegraphed attack. Use the existing alien forms and
effects. Initial tuning targets of roughly 30–60 seconds for an optional finale
are hypotheses for human tests, not mandatory timers or invulnerability padding.

### 3. Accidental patrol damage can erase diplomatic intent

**Every primary run ends with at least one country marked offended; six end
with two and four with three.** The controller excludes neutral patrols from
its target selection. Production code sets the personal offense on damage to a
patrol, and the saves retain those flags. Collateral damage or fire passing
through another target therefore deserves attention. Annexation-induced enemy
patrols can also be legitimately attacked; not every offense in an annexation
run is necessarily accidental. The treaty/succession runs also acquire offenses,
so annexation alone does not explain the result.

There is no current amnesty. A successful treaty can coexist with personal
hostility, making its repair benefit unavailable. That is a coherent simulation
rule, but a single stray hit is a severe cost in a fast horde shooter with beams,
penetration and area attacks.

**Design implication:** clearly separate warning, local retaliation and enduring
national hostility. Show the affected country immediately, distinguish its
patrols at combat distance, and provide an explicit restitution/surrender route.
Deliberate repeated attacks should still have serious consequences. Validate
the rule with blasts, chaining weapons and two-player crossfire.

### 4. Optional encounters work, but rewards lose leverage

The median observed ordinary optional fight bout is **2.97 seconds** across 124
bouts, including incidental encounters on the main route. Deliberately claimed
combat detours have a median total duration of **19.85 seconds**, including
travel and claiming. Tool rewards and directional relics are earned successfully;
several final profiles have a rank-three directional relic.

The weak point is the decision after the character is already powerful. Extra
points and salvage have diminishing value once the stat tree is full. Four
toolbox routes exercise equipment, but this review does not establish that the
equipment is required, faster, or more enjoyable than ordinary shooting.

**Design implication:** give detours an upfront, legible offer: a new play pattern,
a rare specialization, a faction advantage or a route benefit. Add useful repeat
spending before increasing currency income. Make flight, the wagon, the mech and
matrix services solve distinct encounter problems while leaving an ordinary
weapon solution possible. Run paired comparisons from the same earned save
before deciding their numerical advantages.

### 5. The director needs memory of player intent

The twelve runs receive **321 dispatches**: 140 surveys, 133 couriers, 32 supplies
and 16 conflict invitations. Surveys and couriers account for **85%**. Consecutive
repetition protection works, yet alternating those two cards can still feel
repetitive. The controller claims only nearby offers and completes one country
per ship, leaving two unresolved courts; this policy naturally increases courier
repetition. The 57% claim rate is a controller outcome, not a measure of human
interest.

The director functions as a bounded offer system. These results do not show it
creating a larger dramatic arc or adapting enemies, which it was deliberately
designed not to do.

**Design implication:** remember declined or ignored offers per country, reduce
repeat invitations, and connect the next offer to an existing story or political
consequence. Build sequences such as an abandoned procession, a threatened
ambassador and a retaliation encounter. Keep their locations and consequences
persistent. Choose a pacing beat before selecting its event, while retaining
visible warnings, cooldowns and the protection against changing explored rooms.

### 6. World variety has improved; readable scale remains uneven

The sample enters **2,875 room owners**, all 28 themes, the ordinary footprint and
all eight macro forms. Paired-deck volumes occur on multiple routes. On the pure
east and west runs, the second half contains **four cold-weather conditions and
all six enemy ecologies**. The earlier finding of near-uniform cold-weather
ecology is substantially addressed for these routes.

Seed 713's inward route still has a strong cryogenic bias: 112 of 180 owner rooms
newly entered in its second half use rime air. That is one ship identity/route,
not evidence that every ship is cryogenic. The twelve-seed sample is much more
varied than the old single-seed review.

The rendered evidence is less uniformly successful than the content counts.
Rain, fog, colored light, low resolution and enormous dark spaces establish the
tone. Some framed approaches still read as a wall or a long corridor. Focal
structures can be off the walking route, partly occluded, or hard to distinguish
from the background. Looking toward an object's known coordinates does not
guarantee a good view of it; those unsuccessful captures are retained too.

**Design implication:** protect a useful sightline from a room's actual entrance
and landing. Give large spaces a readable near/middle/far composition: a railing
or figure nearby, a bridge or moving machine midway, and restrained distant
lights. Preserve darkness between those elements. Create regional sequences
with a recognizable destination and a small local event instead of relying on
more room-name permutations alone.

### 7. The recovery controller was a significant source of bad exploration

Run 10 stops making meaningful progress around minute 29.6. Its original recovery
policy strafes while still moving forward into furnishings. It repeats 270 times
and abandons ten goals, ending at layer four. This is a failed deep exploration
result even though the simulation continues for sixty minutes.

The thirteenth run changes **only those recovery inputs**: back away for one
second, then strafe for one second. It has 201 identical path/combat observations
through the first recovery, then reaches **layer 25**, enters 330 room owners,
claims eight optional battles and has **one recovery / no abandoned goals**.
No game code, position, damage, health or inventory is changed.

| Same seed 2026 / same route and build | Original | Back-away recovery |
| --- | ---: | ---: |
| Simulated duration | 60 min | 60 min |
| Maximum layer | 4 | 25 |
| Recovery attempts | 270 | 1 |
| Abandoned goals | 10 | 0 |
| Room owners entered | 95 | 330 |
| Kills | 452 | 1,291 |

The first recovery positions from runs 04 and 05 are also reproduced with
unchanged-prefix diagnostic runs. Normal four-second movement trials escape
their corners by up to 28.7 and 30 metres. The final run-10 corner also permits
ordinary escape. These are **not demonstrated geometry softlocks**. Other
recovery locations have not all been diagnosed individually.

The logged maximum stall of about five seconds is the controller's resettable
counter, not the duration of a prolonged failure. The repeated recovery events
are necessary to see its true extent. Improve local obstacle routing and retain
this negative evidence before expanding unattended balance conclusions.

### 8. The interface competes with the scene

The maps, door cues, court labels and reward indicators expose the systems, but
several captures have overlapping or very large world signs. A central interaction
bar crosses the court figures or a focal structure. The same simple weapon model
also occupies substantial screen space across otherwise different weapons.

**Design implication:** prioritize one contextual prompt, separate status messages
from world labels, limit sign size with distance, and let the world remain visible
while a player is within interaction range. Test readable hostile/friendly markers
and threat cues against actual dark backgrounds. The late-run navigation cue in
these captures follows the game's tracked story lead; the controller's independent
exploration waypoint is not a player-selected tracked mission.

## Revised implementation priorities

These are recommendations, **not implemented changes or a newly approved scope**.

| Order | Proposed work | Evidence / acceptance check |
| --- | --- | --- |
| 1 | Tune growth and recovery together; slow universal stat completion and test partial upgrade refills | Same earned builds and seeds, tick-level damage, resource lows, time to stat cap; preserve fast fodder kills and forgiving recovery |
| 2 | Strengthen phase and royal finales through distinctive attack sequences and squad roles | Each boss gets to demonstrate its readable mechanics against several earned loadouts; avoid one blanket health multiplier |
| 3 | Make collateral fire and diplomacy recoverable and legible | Blast/beam/chain and co-op tests; a warning and explicit restitution path before an accidental hit permanently defines the expedition |
| 4 | Improve reward decisions and repeat spending | Players can explain why a detour matters after minute 30; extra points and salvage retain a useful purpose |
| 5 | Extend director memory and linked event sequences | Track country-specific declined offers and repeated pairs; verify at least three meaningfully different pacing beats in a 30-minute human session |
| 6 | Give regions a compact exploration arc | A landmark, inhabited/contested approach, local story, consequential choice and visible aftermath across several rooms |
| 7 | Improve encounter composition and tool opportunities | A dangerous, readable specialist supported by fragile hordes; paired gun/tool tests including matrix, flight, mech and wagon |
| 8 | Protect entrance sightlines and scale cues | Enter real rooms from normal doors/lifts; the focal structure is visible without a debug camera or knowledge of its coordinates |
| 9 | Clarify court, patrol and objective presentation | No overlapping critical labels; readable allegiance during combat; one dominant contextual interaction |
| 10 | Improve the automated controller, then repeat a smaller controlled balance matrix | Local obstacle planning, backward recovery, bounded perception and matched seed/build comparisons; retain all failed routes |
| 11 | Human first-hour and optional co-op review | Time spent lost, perceived danger, memorable discoveries, reward decisions and desire to start a new ship; physical input/device checks separately |

The recommended next milestone groups **1–4** into a focused balance pass. World
expansion and director sequences should then use the resulting power curve.
Do not use zero automated deaths as a target for harsher death penalties; the
desired game is still a powerful, forgiving horde crawler.

## Revisiting the previous reviews

| Previous concern | Current assessment |
| --- | --- |
| Hot/cold atmosphere and inhabitants become monotonous | Substantially improved on sampled lateral routes; four weather conditions and six ecologies in each pure east/west late segment |
| Navigation stalls imply the generator is broken | Not supported by the diagnosed cases; the controlled recovery comparison shows a major controller limitation |
| More alien behaviors and tool encounters needed | Implemented and present; short fight lifetimes now limit how much they can affect a strong build |
| Optional rewards should justify exploration | Rewards and relic advancement function; stat saturation reduces their lasting value |
| Giant spaces need scale and framing | Focal structures and volume variation exist; ordinary entry views still need deliberate sightlines and contrast |
| Politics/director need functional evidence | All twelve disputes settle and persist; replay value, consequences and pacing now need tuning |
| Late-game challenge is too weak for a strong automated build | Still supported across the varied sample; royal finales and growth deserve first attention |

This updates the [earlier deep review](../2026-11-06-deep-expeditions/README.md),
[deep-frontier evidence](../2026-11-07-deep-frontiers/README.md), and
[political implementation evidence](../2026-11-07-alien-courts/README.md).
Raw kill rates are not a before/after balance experiment: this controller has
imperfect aiming, different routes, optional visits and different spending.

## Screenshots and evidence

The [screenshot gallery](screenshots/README.md) links real Chromium/WebGL frames
from earned checkpoints, with matching JSON provenance. Replays use ordinary
mouse/movement/USE controls and do not inject state. Some contain a few seconds
of resumed simulation or walking to frame the view; they are not exact frames
from the native headless run. Save/resume resets transient projectiles and
reconstructs encounters according to normal game rules. Dark or obstructed views
are retained because they are useful review evidence.

All **291 retained checkpoint files from the thirteen long runs** load through
the production loader without a world failure. All thirteen perform a real
save/load round trip after minute 30. There are no world-allocation/stream-budget
failures observed by the controller. The two extra diagnostic prefixes and
four-second escape probes are excluded from the thirteen-hour total.

The driver and diagnostic C tools compile with C17, optimization and
`-Wall -Wextra -Werror`; Python tools compile; strict
[OpenSpec validation](data/spec-validation.log) passes all 19 items. These are
gameplay/review observations, not additions to unit line coverage. The shipped
core and renderer are unchanged, so the full previous unit/browser matrix was
not rerun for this documentation-only review. The saved-frame browser replays
are actual WebGL executions with seed/rank/generator and console-error checks.

## Method and limitations

- **Offense:** 0.30-second reaction, 2.8 rad/s turn limit, 0.025-radian aim error;
  damage-first spending immediately as it becomes affordable; prefers the carbine.
- **Survival:** 0.55 seconds, 1.8 rad/s, 0.040 radians; vitality/shield first,
  spending at quiet thirty-second opportunities; uses a second equipped weapon
  when suitable.
- **Toolbox:** 0.40 seconds, 2.3 rad/s, 0.035 radians; damage/reactor priorities,
  quiet spending, earned weapon preferences and situational earned tools.

All upgrades pay their normal costs; all rewards, phase gates, enemies and
travel use normal gameplay actions. Only journal closing uses the same direct
presentation flag as the existing journey tests. No equipment, health, damage,
positions, kills or world records are injected. Source and binary hashes,
commands and wall durations are retained in each `execution.json`.

These controllers know map plans, continuously move and locate visible threats
through read-only world data. Their deterministic aim error is much simpler than
human perception and panic. Seeds/routes/builds vary together, so the twelve-run
table cannot isolate the causal effect of a build. Death recovery is not exercised
because no character dies. Shelter coverage is a brief safe rest and tool refit,
not bed/kitchen/medical strategy. Matrix dives, strategic flares, ongoing narrative
chapters, all three disputes in one ship, audio quality, human surprise and
physical Windows/mobile/co-op play are not comprehensively evaluated here.
Story choices use option one; political choices vary. No claim is made that
every feature, species or weapon has been playtested.

From the repository root, with the native core built:

```sh
cc -std=c17 -O2 -Wall -Wextra -Werror -Isrc \
  docs/reviews/2026-11-07-long-expeditions/data/review-driver.c \
  build/libsc_core.a -lm -o /tmp/sc-long-review
cc -std=c17 -O2 -Wall -Wextra -Werror -Isrc \
  docs/reviews/2026-11-07-long-expeditions/data/inspect-checkpoint.c \
  build/libsc_core.a -lm -o /tmp/sc-review-inspect
python3 docs/reviews/2026-11-07-long-expeditions/data/run.py \
  --jobs 2 --output /tmp/starship-review-repeat
MPLCONFIGDIR=/tmp/sc-review-mpl python3 \
  docs/reviews/2026-11-07-long-expeditions/data/summarize.py
```

`summarize.py` rebuilds summaries/plots from the retained evidence, validates its
saves, and includes the separate recovery comparison when present. Plotting needs
Matplotlib. `replay.py` needs the Playwright/Chromium setup described in
[TESTING.md](../../../TESTING.md) and a packaged current web build; its `--site`
option selects that package. Each image's JSON records the exact source save,
WASM hash, camera request and movement duration. The recovery scripts retain the
precise observational addition or two-line input change used in their comparisons.
