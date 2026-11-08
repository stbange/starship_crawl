# Measured update costs

All update times below are microseconds. Native uses one 600-second simulated expedition; browser uses 60 simulated seconds at an earned occupied-station checkpoint. Review-only linker wrappers time the ordinary core. Component percentiles cannot be summed; the complete game step includes ordinary enemies, projectiles, streaming and the component calls.

| Build / measurement | Samples | p50 µs | p95 µs | p99 µs | Max µs |
| --- | ---: | ---: | ---: | ---: | ---: |
| Native: Strategy call, including idle frames | 36000 | 0.08 | 0.23 | 0.62 | 654.64 |
| Native: Physical force movement | 36000 | 6.12 | 150.43 | 403.69 | 3826.26 |
| Native: Director | 36000 | 3.48 | 21.33 | 35.23 | 1076.78 |
| Native: Actual two-second strategy update | 290 | 53.48 | 417.48 | 613.46 | 654.64 |
| Native: Faction targeting/combat | 36000 | 1.01 | 134.48 | 165.67 | 3621.05 |
| Native: Complete game step | 36000 | 1146.39 | 2172.80 | 2859.21 | 12880.03 |
| Chromium / WebGL: Strategy call, including idle frames | 3632 | 0.00 | 0.00 | 100.00 | 25100.00 |
| Chromium / WebGL: Physical force movement | 3632 | 200.00 | 1400.00 | 8200.00 | 28400.00 |
| Chromium / WebGL: Director | 3632 | 0.00 | 100.00 | 200.00 | 20100.00 |
| Chromium / WebGL: Actual two-second strategy update | 31 | 100.00 | 4600.00 | 25100.00 | 25100.00 |
| Chromium / WebGL: Faction targeting/combat | 3632 | 300.00 | 4300.00 | 9200.00 | 28500.00 |
| Chromium / WebGL: Complete game step | 3632 | 1900.00 | 10200.00 | 25700.00 | 67600.00 |

Browser animation-frame intervals (excluding the initial zero sample): 602 samples; p50 200.00 ms, p95 266.70 ms, p99 300.10 ms. These are software-rendered Chromium measurements under the cloud CPU quota, not a physical GPU/mobile frame-rate claim.

The browser timer is quantized at approximately 100 microseconds here: zero-valued samples mean below its resolution, not free work. Only 31 actual strategy ticks occur in the web probe, so its high percentiles are exploratory. The native and browser probes cover different scenes and are not a controlled platform-speed comparison.

Browser memory and state: `{"faction_bytes": 54896, "game_bytes": 2924432, "region_bytes": 72, "region_capacity": 32, "regions": 32, "wasm_bytes": 67108864}`.

Native harness/launcher process measurement: `{   "returncode": 0,   "wall_seconds": 44.176411585001915,   "user_seconds": 44.115717,   "system_seconds": 0.03245,   "max_rss_kib": 15616,   "conditions": "single expedition after the local browser/render queue; sanitizer checks may run concurrently" }`.

The native probe starts after the browser queue; sanitizer checks can run concurrently. The earlier hour-run timings were taken during parallel testing and include scheduler contention. Neither set is a latency guarantee. The 32-force, 20-actor-per-room and fourteen-site work limits were retained after bounded-work, full-room and separated co-op tests; physical Windows/mobile profiling remains pending.
