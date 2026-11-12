// Compiled only into starship_crawl_quest.js. Uses standard WebXR APIs; the
// test device lives in tests/, never in the application or shared game core.
(function () {
  'use strict';
  const el = id => document.getElementById(id);
  const prefKey = 'starship-crawl.quest.preferences.v1';
  let ready = false, supported = false, starting = false, session = null;
  let space = null, layer = null, contextLost = false, begun = false, graphicsStarted = false;
  let binding = null, projection = null, eyeTarget = null, eyeDepth = null;
  let depthWidth = 0, depthHeight = 0;
  let lastError = '';
  let watchdog = 0, lastFrameAt = 0, startedAt = 0, trackingReady = false, stopping = false;
  const health = {phase: 'Loading', frames: 0, eyes: 0, poses: 0, controllers: [],
    visibility: 'none', frameMs: 0, framebuffer: '', graphicsStage: '', framebufferStatus: '',
    renderTarget: '', enabledFeatures: [], startupGlErrors: [], error: '', failurePhase: '', logs: []};
  function report() {
    el('xr-report').textContent = JSON.stringify({build: 'quest-projection-framebuffer-3',
      browser: navigator.userAgent, ...health}, null, 2);
  }
  const message = text => { el('status').textContent = text; report(); };
  function phase(text) { health.phase = text; message(text); }
  function stopWithError(reason) {
    if (stopping) return;
    stopping = true;
    health.failurePhase = health.phase;
    lastError = reason; health.error = reason; phase(reason);
    el('diagnostics').open = true;
    try { suspend(); } catch (error) { health.logs.push(String(error).slice(0, 1000)); }
    const current = session;
    if (current) current.end().catch(() => cleanSession(current));
  }
  function watchSession() {
    if (!session || trackingReady || stopping || contextLost) return;
    const now = performance.now();
    if (session.visibilityState !== 'visible') {
      lastFrameAt = startedAt = now;
      return;
    }
    if (now - lastFrameAt > 12000) {
      stopWithError('VR stopped receiving frames. Return to the page and try Enter VR again. The troubleshooting report below records where it stopped.');
    } else if (!trackingReady && now - startedAt > 20000) {
      stopWithError('VR could not finish tracking setup. Hold both Touch controllers in view, face forward and try Enter VR again.');
    }
  }
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
    clearInterval(watchdog); watchdog = 0;
    if (begun) {
      begun = false;
      try { _sc_xr_end(); } catch (error) { health.logs.push(String(error).slice(0, 1000)); }
    }
    releaseTargets();
    session = space = layer = null; starting = false;
    health.phase = lastError ? 'Stopped with an error' : 'Session ended';
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
  function releaseTargets() {
    if (!contextLost) {
      if (eyeTarget) GLctx.deleteFramebuffer(eyeTarget);
      if (eyeDepth) GLctx.deleteRenderbuffer(eyeDepth);
    }
    if (projection && projection.destroy) {
      try { projection.destroy(); } catch (error) { health.logs.push(String(error).slice(0, 1000)); }
    }
    binding = projection = eyeTarget = eyeDepth = null;
    depthWidth = depthHeight = 0;
  }
  function createTargets(current) {
    const scale = preferences().scale;
    health.enabledFeatures = Array.from(current.enabledFeatures || []);
    const layersGranted = !current.enabledFeatures || health.enabledFeatures.includes('layers');
    if (layersGranted && typeof XRWebGLBinding !== 'undefined' &&
        typeof XRWebGLBinding.prototype.createProjectionLayer === 'function') {
      try {
        binding = new XRWebGLBinding(current, GLctx);
        projection = binding.createProjectionLayer({textureType: 'texture', colorFormat: GLctx.RGBA8,
          depthFormat: GLctx.DEPTH_COMPONENT24, scaleFactor: scale});
      } catch (error) {
        releaseTargets();
        if (error.name !== 'NotSupportedError') throw error;
        health.logs.push('Projection layers unavailable: ' + error.message);
      }
      if (projection) {
        eyeTarget = GLctx.createFramebuffer();
        if (!eyeTarget) throw new Error('Could not allocate the headset projection target.');
        current.updateRenderState({layers: [projection], depthNear: 0.05, depthFar: 12000});
        health.renderTarget = 'projection-textures';
        health.framebuffer = projection.textureWidth + ' × ' + projection.textureHeight;
        return;
      }
    }
    layer = new XRWebGLLayer(current, GLctx, {alpha: false, antialias: false, depth: true,
      framebufferScaleFactor: scale});
    current.updateRenderState({baseLayer: layer, depthNear: 0.05, depthFar: 12000});
    health.renderTarget = 'base-layer';
    health.framebuffer = layer.framebufferWidth + ' × ' + layer.framebufferHeight;
  }
  function bindProjection(view) {
    const image = binding.getViewSubImage(projection, view);
    if (!image || !image.colorTexture)
      throw new Error('The headset did not provide a ' + view.eye + '-eye projection color texture.');
    GLctx.bindFramebuffer(GLctx.FRAMEBUFFER, eyeTarget);
    GLctx.framebufferTexture2D(GLctx.FRAMEBUFFER, GLctx.COLOR_ATTACHMENT0,
      GLctx.TEXTURE_2D, image.colorTexture, 0);
    if (image.depthStencilTexture) {
      GLctx.framebufferTexture2D(GLctx.FRAMEBUFFER, GLctx.DEPTH_ATTACHMENT,
        GLctx.TEXTURE_2D, image.depthStencilTexture, 0);
    } else {
      const width = image.colorTextureWidth || projection.textureWidth;
      const height = image.colorTextureHeight || projection.textureHeight;
      if (!Number.isInteger(width) || !Number.isInteger(height) || width < 1 || height < 1 ||
          width > GLctx.getParameter(GLctx.MAX_RENDERBUFFER_SIZE) ||
          height > GLctx.getParameter(GLctx.MAX_RENDERBUFFER_SIZE))
        throw new Error('The headset projection size is invalid.');
      if (!eyeDepth) eyeDepth = GLctx.createRenderbuffer();
      if (!eyeDepth) throw new Error('Could not allocate headset depth storage.');
      GLctx.bindRenderbuffer(GLctx.RENDERBUFFER, eyeDepth);
      if (width !== depthWidth || height !== depthHeight) {
        GLctx.renderbufferStorage(GLctx.RENDERBUFFER, GLctx.DEPTH_COMPONENT24, width, height);
        depthWidth = width; depthHeight = height;
      }
      GLctx.framebufferRenderbuffer(GLctx.FRAMEBUFFER, GLctx.DEPTH_ATTACHMENT,
        GLctx.RENDERBUFFER, eyeDepth);
    }
    const status = GLctx.checkFramebufferStatus(GLctx.FRAMEBUFFER);
    health.framebufferStatus = '0x' + status.toString(16);
    if (status !== GLctx.FRAMEBUFFER_COMPLETE)
      throw new Error('The projection framebuffer is incomplete (' + health.framebufferStatus + ').');
    return image.viewport;
  }
  function xrFrame(time, frame) {
    const current = session;
    if (!current || frame.session !== current || contextLost || stopping) return;
    const frameStart = performance.now();
    lastFrameAt = frameStart;
    health.visibility = current.visibilityState;
    try {
      current.requestAnimationFrame(xrFrame);
      const pose = frame.getViewerPose(space);
      if (!pose || pose.emulatedPosition || !pose.views.length || current.visibilityState !== 'visible') {
        suspend();
        phase(current.visibilityState !== 'visible' ? 'VR paused by the headset. Return to the game, release controls and press A.' :
          'Waiting for headset tracking. Look around in your play area.');
        return;
      }
      health.poses = 1;
      health.controllers = Array.from(current.inputSources, input => ({hand: input.handedness,
        ray: input.targetRayMode, mapping: input.gamepad ? input.gamepad.mapping : 'no gamepad',
        profiles: Array.from(input.profiles || [])}));
      if (!graphicsStarted) {
        health.graphicsStage = 'Before first XR draw';
        // GL errors are sticky. A setup-time framebuffer error says nothing
        // about this callback's eye target. Retain it in the report, then
        // require a complete target AND error-free clear/draw below. Other
        // errors (including context loss/allocation failures) remain fatal.
        for (let i = 0, error; i < 8 && (error = GLctx.getError()) !== GLctx.NO_ERROR; i++)
          health.startupGlErrors.push('0x' + error.toString(16));
        if (health.startupGlErrors.length) {
          health.logs.push('Before first XR draw: GL ' + health.startupGlErrors.join(', '));
          if (health.startupGlErrors.some(error => error !== '0x506'))
            throw new Error('Graphics setup failed: GL ' + health.startupGlErrors.join(', '));
        }
        if (!projection) {
          // Opaque XR framebuffers may only be checked/used inside this callback.
          health.graphicsStage = 'XR framebuffer binding';
          GLctx.bindFramebuffer(GLctx.FRAMEBUFFER, layer.framebuffer);
          const status = GLctx.checkFramebufferStatus(GLctx.FRAMEBUFFER);
          health.framebufferStatus = '0x' + status.toString(16);
          if (!layer.framebuffer || status !== GLctx.FRAMEBUFFER_COMPLETE)
            throw new Error('The headset framebuffer is incomplete (' + health.framebufferStatus + ').');
        }
        graphicsStarted = true;
      }
      if (projection) {
        health.graphicsStage = 'XR projection framebuffer binding';
        for (const view of pose.views) bindProjection(view);
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
      health.graphicsStage = 'XR frame preparation';
      const drawable = _sc_xr_frame(time, valid, mask, lx, ly, rx, ry);
      health.poses = valid;
      // C draws into the JS binding. Runtime images never enter Emscripten's
      // application-owned object table and are used only in this XR callback.
      if (!projection) GLctx.bindFramebuffer(GLctx.FRAMEBUFFER, layer.framebuffer);
      if (!drawable) {
        phase('Waiting for a usable headset pose. Face forward to calibrate.');
        GLctx.disable(GLctx.SCISSOR_TEST); GLctx.depthMask(true); GLctx.clearColor(0,0,0,1);
        for (const view of projection ? pose.views : [null]) {
          if (projection) bindProjection(view);
          GLctx.clear(GLctx.COLOR_BUFFER_BIT | GLctx.DEPTH_BUFFER_BIT);
        }
        const error = GLctx.getError();
        if (error !== GLctx.NO_ERROR) throw new Error('XR clear failed: GL 0x' + error.toString(16));
        return;
      }
      for (const view of pose.views) {
        health.graphicsStage = view.eye + ' eye';
        const viewport = projection ? bindProjection(view) : layer.getViewport(view);
        HEAPF32.set(view.projectionMatrix, offset + 80);
        HEAPF32.set(view.transform.inverse.matrix, offset + 96);
        health.graphicsStage = view.eye + ' eye';
        if (!_sc_xr_eye(viewport.x, viewport.y, viewport.width, viewport.height))
          throw new Error('The stereo renderer could not draw a valid eye view.');
        health.eyes++;
      }
      health.frames++;
      health.graphicsStage = 'Frame complete';
      health.frameMs = Math.round((performance.now() - frameStart) * 10) / 10;
      if (drawable === 2 && !trackingReady) {
        trackingReady = true;
        // This bounds initial entry, not later deliberate pauses or recovery.
        clearInterval(watchdog); watchdog = 0;
      }
      if (health.frames === 1 || health.frames % 72 === 0 || drawable !== 2) {
        phase(drawable === 2 ? 'VR active. If paused, release controls and press A. Y opens the menu.' :
          'Waiting for both Touch controllers. Hold them in view, release controls, then press A.');
      }
    } catch (error) {
      const target = projection ? eyeTarget : layer && layer.framebuffer;
      if (target && !contextLost && GLctx.getParameter(GLctx.DRAW_FRAMEBUFFER_BINDING) === target)
        health.framebufferStatus = '0x' + GLctx.checkFramebufferStatus(GLctx.FRAMEBUFFER).toString(16);
      stopWithError('VR stopped: ' + error.message);
    } finally {
      if (!contextLost) {
        GLctx.disable(GLctx.SCISSOR_TEST);
        if (projection && eyeTarget) {
          GLctx.bindFramebuffer(GLctx.FRAMEBUFFER, eyeTarget);
          GLctx.framebufferTexture2D(GLctx.FRAMEBUFFER, GLctx.COLOR_ATTACHMENT0, GLctx.TEXTURE_2D, null, 0);
          GLctx.framebufferTexture2D(GLctx.FRAMEBUFFER, GLctx.DEPTH_ATTACHMENT, GLctx.TEXTURE_2D, null, 0);
        }
        GLctx.bindFramebuffer(GLctx.FRAMEBUFFER, null);
      }
    }
  }
  async function enter() {
    if (!ready || !supported || starting || session || contextLost) return;
    starting = true; lastError = ''; stopping = false; trackingReady = false; graphicsStarted = false;
    Object.assign(health, {frames: 0, eyes: 0, poses: 0, controllers: [], frameMs: 0, framebuffer: '',
      graphicsStage: '', framebufferStatus: '', renderTarget: '', enabledFeatures: [],
      startupGlErrors: [], error: '', failurePhase: '', logs: []});
    phase('Requesting VR permission…'); buttons();
    let requested = null;
    try {
      // Request permission directly in the click gesture, before any await.
      requested = await navigator.xr.requestSession('immersive-vr', {optionalFeatures: ['local-floor', 'layers']});
      session = requested;
      startedAt = lastFrameAt = performance.now();
      watchdog = setInterval(watchSession, 1000);
      requested.addEventListener('end', () => cleanSession(requested));
      requested.addEventListener('visibilitychange', () => {
        if (requested.visibilityState !== 'visible') suspend();
      });
      requested.addEventListener('inputsourceschange', suspend);
      // Stop the ordinary preview before the async graphics transition. It
      // must not draw or consume XR setup errors while the runtime takes over.
      begun = !!_sc_xr_begin();
      if (!begun) throw new Error('The expedition is not ready for VR.');
      phase('Preparing headset graphics…');
      await GLctx.makeXRCompatible();
      if (session !== requested || contextLost) throw new Error('VR session ended during setup.');
      createTargets(requested);
      phase('Preparing headset tracking…');
      try { space = await requested.requestReferenceSpace('local-floor'); }
      catch (_) { space = await requested.requestReferenceSpace('local'); }
      space.addEventListener('reset', suspend);
      if (session !== requested || contextLost) throw new Error('VR session ended during setup.');
      if (requested.supportedFrameRates && Array.from(requested.supportedFrameRates).includes(72) && requested.updateTargetFrameRate)
        requested.updateTargetFrameRate(72).catch(() => {});
      // Start audio from the same deliberate launch action if the browser suspended it.
      const audio = Module['SDL3'] && Module['SDL3'].audioContext;
      if (audio) audio.resume().catch(() => {});
      requested.requestAnimationFrame(xrFrame);
      starting = false; buttons(); phase('Waiting for the first headset view…');
    } catch (error) {
      if (!stopping) health.failurePhase = health.phase;
      lastError = 'Could not enter VR: ' + error.message + ' You can try again.';
      health.error = lastError; health.phase = 'Startup failed';
      el('diagnostics').open = true;
      if (requested && session === requested) {
        try { await requested.end(); } catch (_) {}
        cleanSession(requested);
      }
      starting = false; buttons(); message(lastError);
    }
  }
  window.scQuest = {
    end: () => session ? session.end().catch(error => { message(error.message); }) : Promise.resolve(),
    comfort: enabled => { el('vignette').checked = enabled; preferences(); },
    log: text => { health.logs.push(String(text).slice(0, 1000)); health.logs = health.logs.slice(-8); report(); },
    abort: reason => { if (session) stopWithError('The VR game stopped: ' + reason); }
  };
  el('copy-report').addEventListener('click', async () => {
    report();
    try { await navigator.clipboard.writeText(el('xr-report').textContent); message('VR troubleshooting report copied.'); }
    catch (_) { el('diagnostics').open = true; message('Copy the troubleshooting text below.'); }
  });
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
