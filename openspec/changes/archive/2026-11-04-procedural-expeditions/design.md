## Context

The world is a pure deterministic generator with bounded streaming. Existing missions and saves must continue to work. Story progression must not depend on chunk scheduling or map previews.

## Decisions

- Add a separately versioned C narrative state. History derives from the ship seed and narrative version. Persist scene reservations, observations, completed interactions, tracked lead and final choice.
- Sixteen scene templates cover briefing, five environmental discoveries, three evidence sources, three optional operations, three motivation-specific targets and extraction. Templates have architectural/sector preferences and use guaranteed accessible service positions without rewriting generated geometry.
- The bay briefing reserves a lead. Actual player entry into a ready room records environmental evidence and can reveal an alternative lead. Investigating either source reserves the target and an optional operation. Reservations never move; repeated visits and streaming do not replay rewards.
- Three explicit choices at the target have different consequences. The optional operation unlocks the third choice. A nearby cleared target is required to confirm, including when a journal is open. Extraction pays the conclusion once and leaves exploration available.
- Keep the legacy route for saves without narrative state. Format 7 appends explicit fields; validate the event graph, masks, coordinates, uniqueness and tracked target before transactional replacement. No geometry version bump.
- Journal pages separate character gear and expedition information. Short in-world observations are backed by persistent journal entries; required information stays available. UI adapters call ordinary core actions, including browser menus and controller choice bindings.

## Non-goals

Containment arcs, faction simulation, physical rerouting of ship power, companion escorts, procedural dialogue and post-victory arcs are later milestones in the expanded plan. Existing decorative incident trails remain generator-owned; narrative clues describe local evidence without claiming all damage has one cause.

## Validation

Exercise all motivations and outcomes, alternate discovery order, load-order independence, scene placement, exact-once cooperative rewards, save/resume at each stage, malformed state rollback, legacy fixtures, complete movement/combat campaigns and public UI browser journeys. Preserve the 95% per-file and overall coverage gate.
