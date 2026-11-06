Actual native OpenGL captures of the generator-8 arrival, using seed 713 and the
retro renderer on Mesa. These show the same approach included in the web build.

Looking toward the loading bay (`--scene arrival`):

![Loading bay and nearby hull lights](arrival.png)

Looking along the hull toward distant lamps and open space (`--scene hull`):

![Distant hull lights and surrounding stars](hull.png)

Looking back at the boarding craft and its open hatch (`--scene skiff`):

![Boarding craft against the starfield](boarding-skiff.png)

Generator-9 traversal, captured from the packaged WASM/WebGL build using real
keyboard input and seed 713 (Chromium software rendering):

![Physical switchback stairs](stairs.png)

![Continuous ramp route](ramp.png)

![Player one in transit while player two remains on the loading-bay pad](lift-transit.png)

The [controls and traversal guide](../../FOUNDATIONS.md) includes reproducible
inspection scenes. The lift capture is from a normal expedition; the browser test
then saves during transit and verifies safe recovery at the departure pad.
