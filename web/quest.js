// Compiled only into starship_crawl_quest.js. Uses standard WebXR APIs; the
// test device lives in tests/, never in the application or shared game core.
(function () {
  'use strict';
  const el = id => document.getElementById(id);
  const prefKey = 'starship-crawl.quest.preferences.v1';
  let ready = false, supported = false, starting = false, session = null;
  let space = null, layer = null, framebufferID = 0, contextLost = false, begun = false;
  let lastError = '';
  const message = text => { el('status').textContent = text; };
  try {
    const saved = JSON.parse(localStorage.getItem(prefKey) || '{}');
    if (typeof saved.vignette === 'boolean') el('vignette').checked = saved.vignette;
    if ([0.6, 0.8, 1].includes(saved.scale)) el('quality').value = String(saved.scale);
  } catch (_) { /* Private storage still permits a session. */ }
  function preferences() {
    const value = {vignette: el('vignette').checked, scale: Number(el('quality').value)};
    try { localStorage.setItem(prefKey, JSON.stringify(value)); } catch (_) {}
    if (ready) _sc_xr_settings(value.vignette ? 1 : 0);
    return value;
  }
  function buttons() {
    el('enter').disabled = !ready || !supported || starting || !!session || contextLost;
    el('exit').hidden = !session;
    el('import').disabled = !ready || !!session || starting || contextLost;
    el('export').disabled = !ready || !!session || starting || contextLost;
  }
  async function capability() {
    try { supported = !!navigator.xr && await navigator.xr.isSessionSupported('immersive-vr'); }
    catch (_) { supported = false; }
    buttons();
    if (ready && !session) message(supported ? 'Ready. Face forward, hold both controllers, then enter VR.' :
      'Immersive VR is unavailable here. Open this HTTPS page in Quest Browser, or play the regular web game.');
  }
  window.scReady = function () {
    ready = true; preferences(); capability();
  };
  function cleanSession(ended) {
    if (session !== ended) return;
    if (begun) { begun = false; _sc_xr_end(); }
    // The XR runtime owns this framebuffer. Remove its Emscripten name only.
    if (framebufferID) GL.framebuffers[framebufferID] = null;
    framebufferID = 0; session = space = layer = null; starting = false;
    buttons();
    message(contextLost ? 'Graphics context lost. Reload to recover the last saved expedition.' :
      (lastError || 'VR ended. Your expedition is paused and saved. Enter VR to continue.'));
  }
  function suspend() { if (begun) _sc_xr_suspend(); }
  function axes(source) {
    const values = source && source.gamepad && source.gamepad.axes;
    if (!values) return [0, 0];
    const i = values.length >= 4 ? 2 : 0;
    return [values[i] || 0, values[i + 1] || 0];
  }
  function pressed(source, index) {
    const button = source && source.gamepad && source.gamepad.buttons[index];
    return !!button && (button.pressed || button.value > 0.65);
  }
  function copyPose(frame, target, index, offset) {
    if (!target) return 0;
    const pose = frame.getPose(target, space);
    if (!pose || pose.emulatedPosition) return 0;
    HEAPF32.set(pose.transform.matrix, offset + index * 16);
    return 1 << index;
  }
  function xrFrame(time, frame) {
    const current = session;
    if (!current || frame.session !== current || contextLost) return;
    current.requestAnimationFrame(xrFrame);
    try {
      const pose = frame.getViewerPose(space);
      if (!pose || pose.emulatedPosition || current.visibilityState !== 'visible') {
        suspend(); return;
      }
      const offset = _sc_xr_buffer() >>> 2;
      HEAPF32.set(pose.transform.matrix, offset);
      let valid = 1, left = null, right = null;
      for (const input of current.inputSources) {
        if (input.targetRayMode !== 'tracked-pointer' || !input.gamepad || input.gamepad.mapping !== 'xr-standard') continue;
        if (input.handedness === 'left') left = input;
        if (input.handedness === 'right') right = input;
      }
      valid |= copyPose(frame, left && left.gripSpace, 1, offset);
      valid |= copyPose(frame, right && right.gripSpace, 2, offset);
      valid |= copyPose(frame, right && right.targetRaySpace, 3, offset);
      valid |= copyPose(frame, left && left.targetRaySpace, 4, offset);
      const [lx, ly] = axes(left), [rx, ry] = axes(right);
      const mask = (pressed(right, 0) ? 1 : 0) | (pressed(right, 1) ? 2 : 0) |
        (pressed(right, 4) ? 4 : 0) | (pressed(right, 5) ? 8 : 0) |
        (pressed(left, 4) ? 16 : 0) | (pressed(left, 5) ? 32 : 0) |
        (pressed(left, 0) ? 64 : 0) | (pressed(left, 1) ? 128 : 0) |
        (pressed(left, 3) ? 256 : 0) | (pressed(right, 3) ? 512 : 0);
      const drawable = _sc_xr_frame(time, valid, mask, lx, ly, rx, ry);
      GLctx.bindFramebuffer(GLctx.FRAMEBUFFER, layer.framebuffer);
      if (!drawable) {
        GLctx.disable(GLctx.SCISSOR_TEST); GLctx.clearColor(0,0,0,1);
        GLctx.clear(GLctx.COLOR_BUFFER_BIT | GLctx.DEPTH_BUFFER_BIT); return;
      }
      for (const view of pose.views) {
        const viewport = layer.getViewport(view);
        HEAPF32.set(view.projectionMatrix, offset + 80);
        HEAPF32.set(view.transform.inverse.matrix, offset + 96);
        if (!_sc_xr_eye(framebufferID, viewport.x, viewport.y, viewport.width, viewport.height))
          throw new Error('The stereo renderer could not draw a valid eye view.');
      }
    } catch (error) {
      lastError = 'VR stopped: ' + error.message; suspend();
      current.end().catch(() => cleanSession(current));
    }
  }
  async function enter() {
    if (!ready || !supported || starting || session || contextLost) return;
    starting = true; lastError = ''; buttons();
    let requested = null;
    try {
      // Request permission directly in the click gesture, before any await.
      requested = await navigator.xr.requestSession('immersive-vr', {optionalFeatures: ['local-floor']});
      session = requested;
      requested.addEventListener('end', () => cleanSession(requested));
      requested.addEventListener('visibilitychange', () => {
        if (requested.visibilityState !== 'visible') suspend();
      });
      requested.addEventListener('inputsourceschange', suspend);
      await GLctx.makeXRCompatible();
      if (session !== requested || contextLost) throw new Error('VR session ended during setup.');
      layer = new XRWebGLLayer(requested, GLctx, {alpha: false, antialias: false, depth: true,
        framebufferScaleFactor: preferences().scale});
      requested.updateRenderState({baseLayer: layer, depthNear: 0.05, depthFar: 12000});
      try { space = await requested.requestReferenceSpace('local-floor'); }
      catch (_) { space = await requested.requestReferenceSpace('local'); }
      space.addEventListener('reset', suspend);
      if (session !== requested || contextLost) throw new Error('VR session ended during setup.');
      framebufferID = GL.getNewId(GL.framebuffers);
      GL.framebuffers[framebufferID] = layer.framebuffer;
      begun = !!_sc_xr_begin();
      if (!begun) throw new Error('The expedition is not ready for VR.');
      if (requested.supportedFrameRates && Array.from(requested.supportedFrameRates).includes(72) && requested.updateTargetFrameRate)
        requested.updateTargetFrameRate(72).catch(() => {});
      // Start audio from the same deliberate launch action if the browser suspended it.
      const audio = Module['SDL3'] && Module['SDL3'].audioContext;
      if (audio) audio.resume().catch(() => {});
      requested.requestAnimationFrame(xrFrame);
      starting = false; buttons(); message('VR active. Y opens the in-headset menu.');
    } catch (error) {
      lastError = 'Could not enter VR: ' + error.message + ' You can try again.';
      if (requested && session === requested) {
        try { await requested.end(); } catch (_) {}
        cleanSession(requested);
      }
      starting = false; buttons(); message(lastError);
    }
  }
  window.scQuest = {
    end: () => session ? session.end().catch(error => { message(error.message); }) : Promise.resolve(),
    comfort: enabled => { el('vignette').checked = enabled; preferences(); }
  };
  el('enter').addEventListener('click', enter);
  el('exit').addEventListener('click', () => window.scQuest.end());
  el('vignette').addEventListener('change', preferences);
  el('quality').addEventListener('change', preferences);
  el('reload').addEventListener('click', () => location.reload());
  Module.canvas.addEventListener('webglcontextlost', event => {
    event.preventDefault(); contextLost = true; suspend();
    if (ready) _sc_xr_context_lost();
    if (session) window.scQuest.end();
    buttons(); message('Graphics context lost. Reload to recover the last saved expedition.');
    el('reload').hidden = false;
  });
  document.addEventListener('visibilitychange', () => { if (document.hidden) suspend(); });
  window.addEventListener('pagehide', () => { if (ready && !contextLost) { suspend(); _sc_web_save(); } });
  el('export').addEventListener('click', () => {
    try {
      _sc_web_save();
      const bytes = FS.readFile('/quest-persist/expedition.sav');
      const url = URL.createObjectURL(new Blob([bytes], {type: 'application/octet-stream'}));
      const link = document.createElement('a'); link.href = url; link.download = 'starship-quest.sav';
      link.click(); setTimeout(() => URL.revokeObjectURL(url), 1000);
      message('Quest save exported.');
    } catch (error) { message('Export failed: ' + error.message); }
  });
  el('import').addEventListener('change', async event => {
    const file = event.target.files[0];
    event.target.value = '';
    if (!file || session || starting || contextLost) return;
    if (file.size > 16 * 1024 * 1024) { message('Save is too large. Current expedition retained.'); return; }
    try {
      const bytes = new Uint8Array(await file.arrayBuffer());
      if (session || starting || contextLost) return;
      // Keep the transient upload outside IDBFS: deleting it during an
      // asynchronous sync would race the persistence traversal.
      FS.writeFile('/quest-import.sav', bytes);
      const ok = _sc_xr_import();
      message(ok ? 'Save imported into the Quest expedition.' : 'Invalid or unsupported save. Current expedition retained.');
    } catch (error) { message('Import failed; current expedition retained: ' + error.message); }
  });
  capability();
})();
