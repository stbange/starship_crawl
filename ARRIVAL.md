New ships begin in zero gravity, just outside the open hatch of a small boarding
skiff. Ahead, the loading bay sits in an immense dark hull. Amber and cold blue
lights, panel ribs and long rows of fittings reveal its scale as you look along
the wall. Stars surround every exposed view into space and remain distant as you
move; the hull and cabin conceal them.

Turn around and drift back to enter the skiff. Its warm lamps, two seats, inactive
consoles and shallow lockers establish where you came from. You can return at any
time. It is a stationary starting craft with no services, loot or objectives.
The existing fold-space shelter remains the place for recovery.

Use **WASD** to drift, **Space** to rise and **Ctrl** to descend. Gamepad movement
and mobile movement/rise/descend controls work as elsewhere outside the ship.
The skiff stays in zero gravity. Its open hatch needs no interaction key. Return
through it and head toward the amber lights around the loading bay to board the
main ship. Ordinary gravity resumes inside. **F** toggles your flashlight; fixed
hull and cabin lamps work independently. Optional **F2** split-screen supports
players inside and outside the craft at the same time.

New expeditions use **generator 8**, with the same interior plans and encounter
placements as generator 7 for the same seed. Save format **11**, narrative **3**
and matrix generator **1** are unchanged. Saves made in the cabin resume there;
the second character is retained when loading solo. Older expeditions receive
the new sky and hull presentation but retain their original exterior collision
and positions. Press **N**, then **Enter**, to start a fresh ship with the skiff.

Inspection scenes are `arrival`, `hull`, `space`, `skiff` and `cabin`. For example:

```sh
./build/starship_crawl --seed 713 --scene skiff
```

The same views work in the browser with `?seed=713&scene=skiff`. Scene overrides
start a fresh inspection run with automatic saving disabled. Use a separate
browser profile when preserving an existing expedition matters.

Rendering uses a fixed star catalog, bounded hull details and the existing point
light budget. Skiff collision, projectile blocking and visible furnishings share
one static geometry catalog. It does not add streamed rooms or change exploration
budgets. See [Testing](TESTING.md) for automated arrival and regression checks.
