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
    setupFramebufferError: false, setupInvalidEnum: false, incompleteFrame: false,
    projectionSupported: false, grantLayers: true, projectionWithoutDepth: false, projectionRejected: false,
    missingProjectionTexture: false, missingProjectionEye: '', legacyMissingAttachments: false,
    projectionTargets: new Set(), fallbackDepths: new Set(), runtimeTextures: new Set(),
    runtimeTextureDeletes: 0, legacyLayerCreations: 0, requestedOptions: null,
    failEyeClear: false, failEyeDraw: false,
    frameActive: false, capturing: false, framebufferViolations: [],
    missingPoses: [], emulatedPoses: [],
    head: {x:0,y:1.65,z:0}, left:{x:-.28,y:1.25,z:-.32}, right:{x:.28,y:1.25,z:-.32},
    leftAim:{x:-.28,y:1.25,z:-.32}, rightAim:{x:.28,y:1.25,z:-.32},
    leftPad:controls('left'), rightPad:controls('right'), time:1000,
    step(n=1, dt=1000/72) {
      for (let i=0;i<n;i++) {
        const session=this.session;
        if (!session || !session.callback) throw Error('No scheduled XR frame');
        const callback=session.callback; session.callback=null; this.time+=dt;
      this.frameActive=true;
        try { callback(this.time, {
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
          getPose: space => this.missingPoses.includes(space.kind) ? null :
            ({transform:transform(this[space.kind]),emulatedPosition:this.emulatedPoses.includes(space.kind)})
        }); } finally { this.frameActive=false; }
      }
    },
    button(side,index,down) {
      const b=this[side+'Pad'].gamepad.buttons[index]; b.pressed=!!down; b.value=down?1:0;
    },
    stick(side,x,y) {this[side+'Pad'].gamepad.axes=[0,0,x,y];},
    neutral() {for (const side of ['left','right']) {this.stick(side,0,0); for(let i=0;i<6;i++) this.button(side,i,false);}},
    capture() {
      const {gl,framebuffer,framebufferWidth:w,framebufferHeight:h}=this.layer;
      const previous=gl.getParameter(gl.FRAMEBUFFER_BINDING);
      gl.bindFramebuffer(gl.FRAMEBUFFER,framebuffer);
      // Test instrumentation may inspect the completed image. Application
      // reads/draws still obey the opaque framebuffer's callback lifetime.
      const pixels=new Uint8Array(w*h*4);
      this.capturing=true;
      if(this.layer.projection)
        gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,gl.TEXTURE_2D,this.layer.colorTexture,0);
      try { gl.readPixels(0,0,w,h,gl.RGBA,gl.UNSIGNED_BYTE,pixels); }
      finally { this.capturing=false; gl.bindFramebuffer(gl.FRAMEBUFFER,previous); }
      const canvas=document.createElement('canvas');canvas.width=w;canvas.height=h;
      const ctx=canvas.getContext('2d'), data=ctx.createImageData(w,h);
      for(let y=0;y<h;y++) data.data.set(pixels.subarray((h-y-1)*w*4,(h-y)*w*4),y*w*4);
      ctx.putImageData(data,0,0);return canvas.toDataURL('image/png');
    }
  };
  const proto=WebGL2RenderingContext.prototype;
  const bind=proto.bindFramebuffer, clear=proto.clear;
  function framebufferError(gl) {
    // Queue a real INVALID_FRAMEBUFFER_OPERATION, not a mocked getError result.
    const previous=gl.getParameter(gl.FRAMEBUFFER_BINDING), empty=gl.createFramebuffer();
    bind.call(gl,gl.FRAMEBUFFER,empty);
    clear.call(gl,gl.COLOR_BUFFER_BIT);
    bind.call(gl,gl.FRAMEBUFFER,previous);gl.deleteFramebuffer(empty);
  }
  proto.bindFramebuffer=function(target,buffer) {
    if(target===this.FRAMEBUFFER||target===this.DRAW_FRAMEBUFFER)mock.drawFramebuffer=buffer;
    if(target===this.FRAMEBUFFER||target===this.READ_FRAMEBUFFER)mock.readFramebuffer=buffer;
    return bind.call(this,target,buffer);
  };
  const attach=proto.framebufferTexture2D, attachDepth=proto.framebufferRenderbuffer;
  proto.framebufferTexture2D=function(target,attachment,type,texture,level) {
    const result=attach.call(this,target,attachment,type,texture,level);
    if(mock.layer&&mock.layer.projection&&mock.runtimeTextures.has(texture)&&
       mock.drawFramebuffer!==mock.layer.framebuffer)
      mock.projectionTargets.add(mock.drawFramebuffer);
    return result;
  };
  proto.framebufferRenderbuffer=function(target,attachment,type,buffer) {
    if(mock.projectionTargets.has(mock.drawFramebuffer)&&buffer)
      mock.fallbackDepths.add(buffer);
    return attachDepth.call(this,target,attachment,type,buffer);
  };
  const deleteFramebuffer=proto.deleteFramebuffer, deleteTexture=proto.deleteTexture;
  proto.deleteFramebuffer=function(buffer) {
    mock.projectionTargets.delete(buffer);
    return deleteFramebuffer.call(this,buffer);
  };
  proto.deleteTexture=function(texture) {
    if(mock.runtimeTextures.has(texture))mock.runtimeTextureDeletes++;
    return deleteTexture.call(this,texture);
  };
  const xrTarget=buffer=>mock.layer&&(buffer===mock.layer.framebuffer||mock.projectionTargets.has(buffer));
  for(const name of ['clear','drawArrays','readPixels']) {
    const original=proto[name];
    proto[name]=function(...args) {
      const bound=name==='readPixels'?mock.readFramebuffer:mock.drawFramebuffer;
      if(xrTarget(bound)&&!mock.capturing) {
        if(!mock.frameActive) {
          mock.framebufferViolations.push(name+' outside XR callback');
          framebufferError(this);return;
        }
        if(mock.incompleteFrame||(mock.failEyeDraw&&name==='drawArrays')||(mock.failEyeClear&&name==='clear')) {
          if(name==='drawArrays')mock.failEyeDraw=false;
          if(name==='clear')mock.failEyeClear=false;
          framebufferError(this);return;
        }
      }
      return original.apply(this,args);
    };
  }
  const status=proto.checkFramebufferStatus;
  proto.checkFramebufferStatus=function(target) {
    const bound=target===this.READ_FRAMEBUFFER?mock.readFramebuffer:mock.drawFramebuffer;
    if(xrTarget(bound)&&!mock.capturing&&(!mock.frameActive||mock.incompleteFrame))
      return this.FRAMEBUFFER_UNSUPPORTED;
    return status.call(this,target);
  };
  class Session extends EventTarget {
    constructor() {
      super();this.visibilityState='visible';this.supportedFrameRates=[72,90];this.ended=false;
      this.enabledFeatures=['local-floor'];
      if(mock.projectionSupported&&mock.grantLayers&&mock.requestedOptions.optionalFeatures.includes('layers'))
        this.enabledFeatures.push('layers');
    }
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
    requestSession:(mode,options)=>{
      mock.requestedOptions=options||{optionalFeatures:[]};
      return mock.denied?Promise.reject(new DOMException('Permission denied','NotAllowedError')):
        Promise.resolve(mock.session=new Session());
    }
  }});
  proto.makeXRCompatible=function(){
    if(mock.setupFramebufferError)framebufferError(this);
    if(mock.setupInvalidEnum)this.enable(0xffffffff);
    return Promise.resolve();
  };
  window.XRWebGLLayer=class {
    constructor(session,gl,options){
      if(mock.failLayer)throw Error('layer setup rejected');
      mock.legacyLayerCreations++;
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
      if(mock.legacyMissingAttachments) {
        gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,gl.TEXTURE_2D,null,0);
        gl.framebufferRenderbuffer(gl.FRAMEBUFFER,gl.DEPTH_ATTACHMENT,gl.RENDERBUFFER,null);
      }
      mock.layer=this;
      gl.bindFramebuffer(gl.FRAMEBUFFER,null);
    }
    getViewport(view){return {x:view.eye==='left'?0:640,y:0,width:640,height:640};}
  };
  function projectionTexture(gl,depth=false) {
    const texture=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,texture);
    gl.texImage2D(gl.TEXTURE_2D,0,depth?gl.DEPTH_COMPONENT24:gl.RGBA8,1280,640,0,
      depth?gl.DEPTH_COMPONENT:gl.RGBA,depth?gl.UNSIGNED_INT:gl.UNSIGNED_BYTE,null);
    gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.NEAREST);
    mock.runtimeTextures.add(texture);
    return texture;
  }
  class ProjectionBinding {
    constructor(session,gl) {this.session=session;this.gl=gl;}
    createProjectionLayer(options) {
      if(!this.session.enabledFeatures.includes('layers'))throw new DOMException('Layers not granted','NotSupportedError');
      if(mock.projectionRejected)throw new DOMException('Projection format unsupported','NotSupportedError');
      if(mock.failLayer)throw Error('projection layer setup rejected');
      if(options.textureType!=='texture')throw Error('Test expects 2D projection textures');
      const gl=this.gl;
      const layer={gl,projection:true,options,textureWidth:1280,textureHeight:640,
        framebufferWidth:1280,framebufferHeight:640,framebuffer:gl.createFramebuffer(),
        colorTexture:projectionTexture(gl),
        depthStencilTexture:mock.projectionWithoutDepth?null:projectionTexture(gl,true),
        destroy(){this.destroyed=true;}};
      mock.layer=layer;
      gl.bindFramebuffer(gl.FRAMEBUFFER,layer.framebuffer);
      gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,gl.TEXTURE_2D,layer.colorTexture,0);
      gl.bindFramebuffer(gl.FRAMEBUFFER,null);
      return layer;
    }
    getViewSubImage(layer,view) {
      if(!mock.frameActive)throw new DOMException('Outside XR callback','InvalidStateError');
      return {colorTexture:mock.missingProjectionTexture||mock.missingProjectionEye===view.eye?null:layer.colorTexture,
        depthStencilTexture:layer.depthStencilTexture,
        colorTextureWidth:1280,colorTextureHeight:640,
        viewport:{x:view.eye==='left'?0:640,y:0,width:640,height:640}};
    }
  }
  mock.replaceProjectionTextures=()=>{
    const layer=mock.layer;
    layer.colorTexture=projectionTexture(layer.gl);
    if(layer.depthStencilTexture)layer.depthStencilTexture=projectionTexture(layer.gl,true);
  };
  Object.defineProperty(window,'XRWebGLBinding',{configurable:true,
    get:()=>mock.projectionSupported?ProjectionBinding:undefined});
})();
