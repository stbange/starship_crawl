// Test-only WebXR device. It supplies poses and controllers through standard
// browser APIs and uses a real WebGL framebuffer, without touching game state.
(() => {
  const identity = () => [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
  function transform(p) {
    const yaw = p.yaw || 0, pitch = p.pitch || 0;
    const c = Math.cos(yaw), s = Math.sin(yaw), cp = Math.cos(pitch), sp = Math.sin(pitch);
    const m = [c,0,-s,0, s*sp,cp,c*sp,0, s*cp,-sp,c*cp,0, p.x||0,p.y||0,p.z||0,1];
    const inv = identity();
    for (let i=0;i<3;i++) for (let j=0;j<3;j++) inv[i*4+j]=m[j*4+i];
    for (let i=0;i<3;i++) inv[12+i]=-(inv[i]*m[12]+inv[4+i]*m[13]+inv[8+i]*m[14]);
    return {matrix: new Float32Array(m), inverse: {matrix: new Float32Array(inv)}};
  }
  const controls = side => ({handedness: side, targetRayMode: 'tracked-pointer',
    gripSpace: {kind: side}, targetRaySpace: {kind: side+'Aim'},
    gamepad: {mapping:'xr-standard', axes:[0,0,0,0],
      buttons:Array.from({length:6},()=>({pressed:false,value:0,touched:false}))}});
  const mock = window.xrTest = {
    supported: true, denied: false, localFloor: true, headAvailable: true,
    leftAvailable: true, rightAvailable: true, failLayer: false,
    head: {x:0,y:1.65,z:0}, left:{x:-.28,y:1.25,z:-.32}, right:{x:.28,y:1.25,z:-.32},
    leftAim:{x:-.28,y:1.25,z:-.32}, rightAim:{x:.28,y:1.25,z:-.32},
    leftPad:controls('left'), rightPad:controls('right'), time:1000,
    step(n=1, dt=1000/72) {
      for (let i=0;i<n;i++) {
        const session=this.session;
        if (!session || !session.callback) throw Error('No scheduled XR frame');
        const callback=session.callback; session.callback=null; this.time+=dt;
        callback(this.time, {
          session,
          getViewerPose: () => {
            if (!this.headAvailable) return null;
            const views = [-.032,.032].map((eye,i)=> {
              const base=transform(this.head).matrix;
              const at={...this.head,x:base[12]+eye*base[0],y:base[13]+eye*base[1],z:base[14]+eye*base[2]};
              const projection=identity(); projection[0]=projection[5]=1;
              projection[10]=-1.00001;projection[11]=-1;projection[14]=-.1;projection[15]=0;
              return {eye:i?'right':'left',transform:transform(at),projectionMatrix:new Float32Array(projection)};
            });
            return {transform:transform(this.head),views,emulatedPosition:false};
          },
          getPose: space => ({transform:transform(this[space.kind]),emulatedPosition:false})
        });
      }
    },
    button(side,index,down) {
      const b=this[side+'Pad'].gamepad.buttons[index]; b.pressed=!!down; b.value=down?1:0;
    },
    stick(side,x,y) {this[side+'Pad'].gamepad.axes=[0,0,x,y];},
    neutral() {for (const side of ['left','right']) {this.stick(side,0,0); for(let i=0;i<6;i++) this.button(side,i,false);}},
    capture() {
      const {gl,framebuffer,framebufferWidth:w,framebufferHeight:h}=this.layer;
      gl.bindFramebuffer(gl.FRAMEBUFFER,framebuffer);
      const pixels=new Uint8Array(w*h*4); gl.readPixels(0,0,w,h,gl.RGBA,gl.UNSIGNED_BYTE,pixels);
      const canvas=document.createElement('canvas');canvas.width=w;canvas.height=h;
      const ctx=canvas.getContext('2d'), data=ctx.createImageData(w,h);
      for(let y=0;y<h;y++) data.data.set(pixels.subarray((h-y-1)*w*4,(h-y)*w*4),y*w*4);
      ctx.putImageData(data,0,0);return canvas.toDataURL('image/png');
    }
  };
  class Session extends EventTarget {
    constructor() {super();this.visibilityState='visible';this.supportedFrameRates=[72,90];this.ended=false;}
    get inputSources(){return [...(mock.leftAvailable?[mock.leftPad]:[]),...(mock.rightAvailable?[mock.rightPad]:[])];}
    requestReferenceSpace(type) {
      if(type==='local-floor'&&!mock.localFloor)return Promise.reject(Error('no floor'));
      this.reference=new EventTarget();return Promise.resolve(this.reference);
    }
    updateRenderState(state){this.renderState={...state};}
    updateTargetFrameRate(rate){this.rate=rate;return Promise.resolve();}
    requestAnimationFrame(cb){this.callback=cb;return 1;}
    end(){if(!this.ended){this.ended=true;this.callback=null;this.dispatchEvent(new Event('end'));}return Promise.resolve();}
  }
  Object.defineProperty(navigator,'xr',{configurable:true,value:{
    isSessionSupported:()=>Promise.resolve(mock.supported),
    requestSession:()=>mock.denied?Promise.reject(new DOMException('Permission denied','NotAllowedError')):
      Promise.resolve(mock.session=new Session())
  }});
  WebGL2RenderingContext.prototype.makeXRCompatible=function(){return Promise.resolve();};
  window.XRWebGLLayer=class {
    constructor(session,gl,options){
      if(mock.failLayer)throw Error('layer setup rejected');
      this.gl=gl;this.options=options;
      this.framebufferWidth=1280;this.framebufferHeight=640;
      this.framebuffer=gl.createFramebuffer();gl.bindFramebuffer(gl.FRAMEBUFFER,this.framebuffer);
      const texture=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,texture);
      gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA8,1280,640,0,gl.RGBA,gl.UNSIGNED_BYTE,null);
      gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.NEAREST);
      gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.NEAREST);
      gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,gl.TEXTURE_2D,texture,0);
      const depth=gl.createRenderbuffer();gl.bindRenderbuffer(gl.RENDERBUFFER,depth);
      gl.renderbufferStorage(gl.RENDERBUFFER,gl.DEPTH_COMPONENT16,1280,640);
      gl.framebufferRenderbuffer(gl.FRAMEBUFFER,gl.DEPTH_ATTACHMENT,gl.RENDERBUFFER,depth);
      if(gl.checkFramebufferStatus(gl.FRAMEBUFFER)!==gl.FRAMEBUFFER_COMPLETE)throw Error('Incomplete test XR framebuffer');
      mock.layer=this;
    }
    getViewport(view){return {x:view.eye==='left'?0:640,y:0,width:640,height:640};}
  };
})();
