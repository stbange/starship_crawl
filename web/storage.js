// IndexedDB initialization must finish before C tries to resume an expedition.
window.scStorage = {
  status: 0,
  busy: false,
  pending: false,
  flush: function () {
    if (this.busy) { this.pending = true; return; }
    if (!this.available) { this.status = 4; return; }
    this.busy = true;
    this.status = 2;
    FS.syncfs(false, (error) => {
      this.busy = false;
      this.status = error ? 4 : 3;
      if (error) console.error('Save persistence failed', error);
      if (this.pending) { this.pending = false; this.flush(); }
    });
  }
};
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
  FS.mkdir('/persist');
  addRunDependency('starship-save');
  try {
    FS.mount(IDBFS, {}, '/persist');
    FS.syncfs(true, function (error) {
      window.scStorage.available = !error;
      window.scStorage.status = error ? 0 : 1;
      if (error) console.error('Save storage unavailable', error);
      removeRunDependency('starship-save');
    });
  } catch (error) {
    console.error('Save storage unavailable', error);
    removeRunDependency('starship-save');
  }
});
