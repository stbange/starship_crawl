// A distinct mount point is also a distinct IDBFS database on the same origin.
window.scStorage = {
  status: 0, busy: false, pending: false, available: false,
  flush: function () {
    if (this.busy) { this.pending = true; return; }
    if (!this.available) { this.status = 4; return; }
    this.busy = true; this.status = 2;
    FS.syncfs(false, error => {
      this.busy = false; this.status = error ? 4 : 3;
      if (error) console.error('Quest save persistence failed', error);
      if (this.pending) { this.pending = false; this.flush(); }
    });
  }
};
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
  FS.mkdir('/quest-persist');
  addRunDependency('starship-quest-save');
  try {
    FS.mount(IDBFS, {}, '/quest-persist');
    FS.syncfs(true, error => {
      window.scStorage.available = !error;
      window.scStorage.status = error ? 0 : 1;
      if (error) console.error('Quest save storage unavailable', error);
      removeRunDependency('starship-quest-save');
    });
  } catch (error) {
    console.error('Quest save storage unavailable', error);
    removeRunDependency('starship-quest-save');
  }
});
