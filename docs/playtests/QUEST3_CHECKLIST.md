# Physical Meta Quest 3 acceptance checklist

Status: **not executed on hardware**. Software WebXR emulation is a separate
check and cannot establish comfort or a sustained headset frame rate.

User feedback on the initial deployment: black view and unresponsive controls
on Quest 3. The [startup repair](../reviews/2026-11-08-quest-startup/README.md)
reproduces missing-controller entry in software and adds recovery/reporting.
Physical confirmation of that repair is still pending; the initial report does
not establish which browser/device condition caused the failure.

Record the commit, Quest OS/browser versions, headset refresh rate, chosen render
scale, seated/standing mode, run duration and any thermal throttling. Use the
Quest page `/starship_crawl/quest/` after its successful Actions deployment.

- [ ] Open in Quest Browser over HTTPS; permission denial leaves a working launch
      page; a second attempt enters VR with both Touch controllers.
- [ ] Enter with one controller temporarily unavailable. A readable panel follows
      the head and identifies the waiting controller; bring it into view, release
      controls and press A. Record whether the Meta system menu remains usable.
      If entry still fails, copy the launch page's VR troubleshooting report.
- [ ] Stars surround the hull, ship lights recede into the distance, and eye
      separation/scale feels natural. Turn, roll and lean without a mirrored
      scene, eye swap, camera roll correction or doubled simulation speed.
- [ ] Repeat calibration seated and standing. Try physical and button crouching
      in a narrow tunnel. Recenter without changing your expedition position.
- [ ] Walk from the craft through the bay and back. Check diagonal speed, sprint,
      one snap per stick release, lift travel and vignette on/off. Stop immediately
      if a locomotion option feels uncomfortable and record which motion caused it.
- [ ] Look left while firing right. Check projectile, beam and matrix-pulse aim,
      right-hand muzzle position and left-hand flashlight on surfaces and sprites.
      Reach toward a wall: a gun beyond it must not shoot through it.
- [ ] Read wrist health/shield/armor, rig hull or matrix integrity/bandwidth. Read
      the floating story and reward panels at each render scale; paginate long
      text, change map decks and follow the gold/cyan route markers.
- [ ] Claim a genuinely earned reward, spend an upgrade point, refit at safety,
      use an equipped power, deploy/board/fold earned vehicles, and make a story
      decision using only controllers.
- [ ] Enter the shelter, rest/use stations, connect to its matrix terminal and
      return. Check fades on each transition, vehicle mounting and respawn.
- [ ] Lean into geometry and away from the calibration envelope. Confirm the
      world obscures safely and becomes visible after returning/recentering.
- [ ] Hold fire/move while covering a controller's tracking, removing a controller,
      taking the headset off and opening the Quest system menu. The game must
      pause. Recovery must require release plus A and never replay old input.
- [ ] End and re-enter VR several times. Test save/export/import and reload.
      Open the regular web game on the same origin and confirm separate progress.
- [ ] Play at least 30 minutes across a large vault, crowded fight, heavy fog/rain,
      tentacles, a deep faction battle, shelter and matrix. Measure frame timing
      against the selected refresh interval (13.89 ms at 72 Hz); record dropped
      frames, resolution and temperature behavior. Test the 60% scale if needed.
- [ ] Confirm audio starts after the launch gesture, survives system-menu use,
      and stops or quiets appropriately while paused.

Release follow-ups must be based on these measurements. Native Android/OpenXR,
teleport locomotion, room-scale collision walking and hand tracking are separate
possible future targets, not part of this browser build.
