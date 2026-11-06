# Controls and vertical exploration

New ships use generator 9. Previously saved ships retain their geometry and lift
behavior. Choose **N, Enter** (or **Begin a new ship**) to explore the new shafts;
this starts fresh characters. Saving and resuming the current ship retains them.

## Device settings

Open **F1**, or pause with a controller and press **Back/Select**. Choose the
player, navigate with arrows/D-pad, and adjust with left/right. Enter/Start resumes.
The browser's **Menu → Controls and display** also provides labelled controls.

Each player has independent mouse, touch and stick sensitivity, stick deadzone,
vertical-look inversion, and world resolution. Reset restores only that player's
settings. Resolution choices are Auto (desktop 360p, touch 240p), 180p, 240p, 360p,
and 480p. The HUD stays at display resolution. Lower settings reduce GPU work.
Separate cached render targets let split-screen players choose different settings.

Preferences live in `preferences.cfg` alongside local saves, independently of the
expedition. Browser storage synchronizes them with IndexedDB. Importing a character
save or starting another ship preserves the device settings. If storage is blocked,
the menu reports that the change applies only to this session. Malformed preference
files fall back to defaults. Inspection scenes and `--no-save` leave stored settings
alone.

A controller retains its player when the other controller disconnects. Explicitly
changing the player count or attaching a controller assigns owners; unplugging a
controller pauses play. Focus loss/backgrounding pauses and releases held input.
Resizing, rotating and touch cancellation clear pending gestures, so a fresh touch
starts cleanly. Solo play remains the default; F2 enables optional split-screen.

## Walking between decks

The loading bay has a service shaft. **Amber stairs** occupy its east side;
**blue-marked ramps** occupy its west side. Enter from the wide landing toward the
bay mouth. Follow one flight, cross the landing to the adjoining lane, and reverse
direction. Eight flights rise 64 metres to the next deck. Both routes can be walked
up and down under normal gravity, with no jump or power required.

A seeded column in each horizontal four-by-four region continues through the decks.
It contains the same connected landings, stairs, ramps and central grav lift. Doors
and a level apron connect the shaft back to each room. Other cyan pads have compact
lift shafts. Existing sealed phase boundaries and unfinished rooms remain blocked.

Normal walking climbs steps up to 30 centimetres and follows short downward drops.
Headroom still matters: crouch for low passages and stand only where clear. Flight
and airborne jumps do not receive automatic stair assistance. Rails, walls and
ceilings remain solid. Ramp slabs and stair treads also block weapon rays.

## Grav lifts

Stand on a **cyan pad**, then hold **E** to go up or **Q** to go down (controller
X/Y; mobile up/down controls). An available lift carries that player to the adjacent
deck over three seconds. The moving cradle, shaft lights, destination and progress
indicator show the journey. Looking remains available. Walking, firing, deploying,
refitting and entering other spaces wait until arrival.

The destination must be loaded, open for the current phase and clear for the rider
or vehicle. Arrival updates the checkpoint. Co-op players travel independently.
Saving mid-journey stores the departure pad for the rider and their occupied rig;
loading starts there without an active ride. Failed saves/imports leave the live
ride intact. Leaving co-op cancels that participant's ride safely. Death uses the
normal checkpoint recovery.

## Repeatable inspection

Native `--scene stairs`, `--scene ramp`, and `--scene lift`, or browser query strings
`?seed=713&scene=stairs`, `?seed=713&scene=ramp`, and `?seed=713&scene=lift`, open the
new architecture. These are ordinary gameplay scenes with saving disabled, useful
for inspecting and recording traversal. Other historical indoor inspection scenes
keep their reproducible older layouts.

## Physical-device checklist (not completed in cloud)

Use a fresh ship and record OS/browser/GPU, controller model, resolution and whether
the following checks passed. Automated virtual input and software rendering do not
establish these hardware results.

1. **Windows native:** run the Windows build, enter the bay, walk a complete stair
   and ramp route both ways, ride a lift, Alt-Tab while moving/firing, resize and
   return. Confirm rendering, focus recovery and settings after restarting.
2. **Two physical gamepads:** join with F2, move/aim both players, unplug and replug
   each controller in turn, then return to solo. Confirm that the remaining device
   keeps its player. Adjust deadzone/inversion separately and confirm persistence.
3. **Safari desktop:** open the Pages game, capture/release the mouse, change
   settings, save/reload/import, switch tabs while holding movement and ride a lift.
   Confirm controls, WebGL output and storage status.
4. **Android/iOS web:** move/aim/fire with simultaneous touches; rotate while holding
   them, lock/unlock or background the page, then start new gestures. Test menu
   accessibility, lift buttons and a stair route. Compare Auto and 180p performance.
5. **Performance:** record frame pacing in the bay, a great hall and a service shaft,
   with solo and split-screen where supported. Check each quality setting and note
   prolonged stutters or input lag. Report observations rather than assumed FPS.
