"""Run the isolated Quest WASM with a test-only WebXR device and real WebGL."""
import argparse
import base64
from functools import partial
from http.server import ThreadingHTTPServer
import io
import json
import os
from pathlib import Path
import threading
from PIL import Image, ImageChops, ImageStat
from playwright.sync_api import sync_playwright
from test_browser import QuietHandler, ROOT

ARTIFACTS = ROOT / 'build-quest/test-artifacts'


def state(page):
    return json.loads(page.evaluate("Module.ccall('sc_web_state','string',[],[])"))


def xr_state(page):
    return json.loads(page.evaluate("Module.ccall('sc_xr_state','string',[],[])"))


def ready(page):
    page.wait_for_function("typeof Module !== 'undefined' && !document.getElementById('export').disabled", timeout=90000)
    page.wait_for_function("JSON.parse(Module.ccall('sc_web_state','string',[],[])).pending === 0", timeout=90000)


def step(page, count=1, dt=1000/72):
    page.evaluate('([n,dt])=>xrTest.step(n,dt)', [count, dt])


def press(page, side, index):
    page.evaluate('([s,i])=>xrTest.button(s,i,true)', [side, index]); step(page)
    page.evaluate('([s,i])=>xrTest.button(s,i,false)', [side, index]); step(page)


def choose(page, label):
    for _ in range(100):
        q = xr_state(page)
        if label in q['label']:
            press(page, 'right', 4)
            return
        page.evaluate("xrTest.stick('right',0,1)"); step(page)
        page.evaluate("xrTest.stick('right',0,0)"); step(page)
    raise AssertionError((label, xr_state(page)))


def capture(page, name):
    uri = page.evaluate('xrTest.capture()')
    data = base64.b64decode(uri.split(',')[1])
    (ARTIFACTS / f'{name}.png').write_bytes(data)
    img = Image.open(io.BytesIO(data)).convert('RGB')
    left, right = img.crop((0,0,640,640)), img.crop((640,0,1280,640))
    assert sum(ImageStat.Stat(img).mean) > 4, 'Stereo framebuffer is blank'
    assert ImageChops.difference(left,right).getbbox(), 'Both eye images are identical'
    return img


def enter(page):
    page.locator('#enter').click()
    page.wait_for_function('!!(xrTest.session && xrTest.session.callback)')
    step(page, 20)
    assert xr_state(page)['active'] and not state(page)['paused'], xr_state(page)
    assert page.evaluate("!Module.SDL3?.audioContext || Module.SDL3.audioContext.state==='running'")


def walk(page, x, z, sheltered=False):
    # Head poses and sticks are the only movement inputs, including in fixtures.
    page.evaluate('yaw=>{xrTest.head.yaw=(xrTest.head.yaw||0)+yaw}', state(page)['yaw'])
    step(page)
    for key, target in [('sx' if sheltered else 'x',x),('sz' if sheltered else 'z',z)]:
        for _ in range(160):
            delta=target-state(page)[key]
            if abs(delta)<.3: break
            direction=1 if delta>0 else -1
            page.evaluate('([x,y])=>xrTest.stick("left",x,y)',[direction if key.endswith('x') else 0,-direction if key.endswith('z') else 0])
            step(page,1,50)
        else: raise AssertionError(('walk blocked',key,target,state(page)))
        page.evaluate('xrTest.neutral()');step(page)


def import_save(page, name):
    page.locator('#import').set_input_files(str(ROOT/'tests/fixtures'/name))
    page.wait_for_function("document.getElementById('status').textContent.includes('Save imported')")


def projection_checks(page, url):
    def fresh(settings):
        page.goto(url+'?seed=713');ready(page)
        page.evaluate('settings => Object.assign(xrTest, {projectionSupported:false}, settings)', settings)

    def launch():
        page.locator('#enter').click()
        page.wait_for_function('!!(xrTest.session && xrTest.session.callback)')

    def ended():
        page.wait_for_function("!document.getElementById('enter').disabled")
        assert state(page)['paused'] and not xr_state(page)['active']
        assert page.evaluate('xrTest.projectionTargets.size') == 0
        assert page.evaluate('xrTest.runtimeTextureDeletes') == 0

    # Reproduce the physical report with a real framebuffer whose attachments
    # are absent, rather than returning a synthetic completeness status alone.
    fresh({'legacyMissingAttachments': True})
    launch();step(page);ended()
    report=json.loads(page.locator('#xr-report').text_content())
    assert report['framebufferStatus']=='0x8cd7' and report['eyes']==0,report
    (ARTIFACTS/'projection-legacy-missing.json').write_text(json.dumps(report,indent=2)+'\n')

    fresh({'projectionSupported': True, 'legacyMissingAttachments': True})
    enter(page)
    report=json.loads(page.locator('#xr-report').text_content())
    assert report['renderTarget']=='projection-textures' and report['framebufferStatus']=='0x8cd5',report
    assert page.evaluate("xrTest.requestedOptions.optionalFeatures.includes('layers')")
    assert page.evaluate('xrTest.legacyLayerCreations')==0
    capture(page,'quest-projection-stereo')
    before=xr_state(page);page.evaluate('xrTest.replaceProjectionTextures()');step(page,3)
    after=xr_state(page)
    assert after['frames']==before['frames']+3 and after['eyes']==before['eyes']+6,(before,after)
    capture(page,'quest-projection-replaced-textures')
    assert page.evaluate('''() => {
      const gl=xrTest.layer.gl, target=Array.from(xrTest.projectionTargets)[0];
      gl.bindFramebuffer(gl.FRAMEBUFFER,target);
      const color=gl.getFramebufferAttachmentParameter(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,gl.FRAMEBUFFER_ATTACHMENT_OBJECT_NAME);
      const depth=gl.getFramebufferAttachmentParameter(gl.FRAMEBUFFER,gl.DEPTH_ATTACHMENT,gl.FRAMEBUFFER_ATTACHMENT_OBJECT_NAME);
      gl.bindFramebuffer(gl.FRAMEBUFFER,null);return color===null && depth===null;
    }''')
    assert page.evaluate('xrTest.framebufferViolations')==[]
    (ARTIFACTS/'projection-recovered.json').write_text(json.dumps(report,indent=2)+'\n')
    page.evaluate('scQuest.end()');ended()
    assert page.evaluate('xrTest.layer.destroyed')
    enter(page);page.evaluate('scQuest.end()');ended()

    fresh({'projectionSupported': True, 'projectionWithoutDepth': True,
           'leftAvailable': False, 'rightAvailable': False})
    launch();step(page,4)
    assert xr_state(page)['suspended'] and xr_state(page)['eyes']==8
    capture(page,'quest-projection-depth-recovery')
    assert page.evaluate('xrTest.fallbackDepths.size')==1
    step(page,8)
    assert page.evaluate('xrTest.fallbackDepths.size')==1
    page.evaluate('xrTest.leftAvailable=true;xrTest.rightAvailable=true');step(page,2);press(page,'right',4)
    assert not state(page)['paused']
    page.evaluate('scQuest.end()');ended()
    assert page.evaluate('Array.from(xrTest.fallbackDepths).every(buffer => !xrTest.layer.gl.isRenderbuffer(buffer))')

    for settings in ({'projectionSupported': True, 'grantLayers': False},
                     {'projectionSupported': True, 'projectionRejected': True}):
        fresh(settings);enter(page)
        report=json.loads(page.locator('#xr-report').text_content())
        assert report['renderTarget']=='base-layer' and page.evaluate('xrTest.legacyLayerCreations')==1,report
        page.evaluate('scQuest.end()');ended()

    for eye in ('left','right'):
        fresh({'projectionSupported': True, 'missingProjectionEye': eye})
        before=state(page)['time'];launch();step(page);ended()
        report=json.loads(page.locator('#xr-report').text_content())
        assert eye+'-eye projection color texture' in report['error'] and report['frames']==report['eyes']==0,report
        assert report['poses']==1 and len(report['controllers'])==2,report
        assert state(page)['time']==before
        assert not page.locator('#export').is_disabled()

    fresh({'projectionSupported': True});enter(page)
    before=state(page)['time'];frames=xr_state(page)['frames'];eyes=xr_state(page)['eyes']
    page.evaluate("xrTest.missingProjectionEye='right'");step(page);ended()
    report=json.loads(page.locator('#xr-report').text_content())
    assert 'right-eye projection color texture' in report['error'],report
    assert report['frames']==frames and report['eyes']==eyes and state(page)['time']==before,report

    fresh({'projectionSupported': True});enter(page)
    before=state(page)['time'];frames=xr_state(page)['frames']
    page.evaluate('xrTest.incompleteFrame=true');step(page);ended()
    report=json.loads(page.locator('#xr-report').text_content())
    assert report['error'] and report['frames']==frames and state(page)['time']==before,report
    assert page.evaluate('xrTest.framebufferViolations')==[]
    page.goto(url+'?seed=713');ready(page)
    return 'projection stereo with missing opaque attachments, replaced images, depth fallback, controllers, capability fallback, cleanup and invalid-target refusal'


def framebuffer_checks(page, url):
    def fresh():
        page.goto(url+'?seed=713');ready(page)
        page.evaluate('xrTest.projectionSupported=false')

    fresh()
    page.evaluate('xrTest.setupFramebufferError=true;xrTest.leftAvailable=false;xrTest.rightAvailable=false')
    page.locator('#enter').click()
    page.wait_for_function('!!(xrTest.session && xrTest.session.callback)')
    step(page,4)
    q=xr_state(page)
    assert q['active'] and q['suspended'] and q['eyes']==8,q
    capture(page,'quest-framebuffer-startup-recovery')
    report=json.loads(page.locator('#xr-report').text_content())
    assert report['startupGlErrors']==['0x506'] and not report['error'],report
    (ARTIFACTS/'framebuffer-recovered.json').write_text(json.dumps(report,indent=2)+'\n')
    assert page.evaluate('xrTest.framebufferViolations')==[]
    assert page.evaluate('xrTest.layer.gl.getParameter(xrTest.layer.gl.FRAMEBUFFER_BINDING)===null')
    page.evaluate('xrTest.leftAvailable=true;xrTest.rightAvailable=true');step(page,2)
    press(page,'right',4)
    assert not state(page)['paused']
    page.evaluate('scQuest.end()')
    page.wait_for_function("!document.getElementById('enter').disabled")
    enter(page)
    assert page.evaluate('xrTest.framebufferViolations')==[]
    page.evaluate('scQuest.end()')
    page.wait_for_function("!document.getElementById('enter').disabled")

    failures=[]
    for fault,stage in [('incompleteFrame','XR framebuffer binding'),('failEyeClear','left eye'),
                        ('failEyeDraw','left eye')]:
        fresh();page.evaluate('key=>xrTest[key]=true',fault)
        page.locator('#enter').click()
        page.wait_for_function('!!(xrTest.session && xrTest.session.callback)')
        step(page)
        page.wait_for_function("!document.getElementById('enter').disabled")
        report=json.loads(page.locator('#xr-report').inner_text())
        assert report['error'] and report['graphicsStage']==stage and report['eyes']==0,report
        assert state(page)['paused'] and not xr_state(page)['active']
        assert not page.locator('#export').is_disabled()
        assert page.evaluate('xrTest.framebufferViolations')==[]
        assert not page.evaluate('xrTest.layer.gl.isEnabled(xrTest.layer.gl.SCISSOR_TEST)')
        if fault=='failEyeDraw':
            assert any('draw: GL 0x506' in log for log in report['logs']),report
        if fault=='failEyeClear':
            assert any('clear: GL 0x506' in log for log in report['logs']),report
        failures.append(report)
    # A target that fails after successful entry must still stop immediately;
    # setup-error recovery never applies to subsequent frames or draw errors.
    fresh();enter(page)
    eyes=xr_state(page)['eyes']
    page.evaluate('xrTest.incompleteFrame=true');step(page)
    page.wait_for_function("!document.getElementById('enter').disabled")
    report=json.loads(page.locator('#xr-report').text_content())
    assert report['error'] and report['eyes']==eyes and report['graphicsStage']=='left eye',report
    assert report['framebufferStatus']=='0x8cdd',report
    assert any('draw: GL 0x506' in log for log in report['logs']),report
    assert not page.evaluate('xrTest.layer.gl.isEnabled(xrTest.layer.gl.SCISSOR_TEST)')
    assert state(page)['paused'] and not xr_state(page)['active']
    failures.append(report)
    fresh();page.evaluate('xrTest.setupInvalidEnum=true')
    page.locator('#enter').click()
    page.wait_for_function('!!(xrTest.session && xrTest.session.callback)')
    step(page)
    page.wait_for_function("!document.getElementById('enter').disabled")
    report=json.loads(page.locator('#xr-report').text_content())
    assert report['startupGlErrors']==['0x500'] and report['error'] and report['eyes']==0,report
    assert report['graphicsStage']=='Before first XR draw' and state(page)['paused'],report
    failures.append(report)
    (ARTIFACTS/'framebuffer-diagnostics.json').write_text(json.dumps(failures,indent=2)+'\n')
    fresh()
    return 'opaque framebuffer lifetime, reported setup GL error recovery, head-only stereo, re-entry, genuine incomplete/draw failures and usable exit'


def startup_checks(page, url):
    def fresh():
        page.goto(url+'?seed=713');ready(page)

    def launch():
        page.locator('#enter').click()
        page.wait_for_function('!!(xrTest.session && xrTest.session.callback)')

    def end():
        page.evaluate('scQuest.end()')
        page.wait_for_function("!document.getElementById('export').disabled")

    fresh()
    page.evaluate("xrTest.rightAvailable=false;xrTest.button('right',0,true);xrTest.stick('left',0,-1)")
    before=state(page)
    launch();step(page,20)
    q=xr_state(page)
    assert q['active'] and q['suspended'] and not q['tracking'] and q['valid']&1,q
    assert q['frames']>0 and q['eyes']==2*q['frames'],q
    assert state(page)['time']==before['time'] and state(page)['shots']==before['shots']
    capture(page,'quest-startup-missing-controller')
    page.evaluate('xrTest.head.yaw=1.5');step(page,4)
    capture(page,'quest-startup-recovery-looking-around')
    page.evaluate('xrTest.rightAvailable=true');step(page,4)
    assert state(page)['paused'] and state(page)['shots']==before['shots']
    page.evaluate('xrTest.neutral()');step(page,2)
    assert state(page)['paused']
    press(page,'right',4)
    assert not state(page)['paused'] and state(page)['time']-before['time']<.1
    press(page,'right',0)
    assert state(page)['shots']>before['shots']
    end()

    fresh();page.evaluate("xrTest.missingPoses=['right','leftAim']")
    launch();step(page,12)
    assert xr_state(page)['suspended'] and not xr_state(page)['tracking']
    capture(page,'quest-startup-missing-pose')
    page.evaluate("xrTest.missingPoses=[];xrTest.emulatedPoses=['right']");step(page,3)
    assert xr_state(page)['suspended'] and not xr_state(page)['tracking']
    page.evaluate('xrTest.emulatedPoses=[]');step(page,2)
    press(page,'right',4)
    assert not state(page)['paused']
    end()

    fresh();page.evaluate('xrTest.head.pitch=Math.PI/2')
    enter(page)
    q=xr_state(page)
    assert q['tracking'] and not q['unsafe'] and q['eyes']==2*q['frames'],q
    end()

    # Advance the browser's monotonic clock, not game state. One real timer tick
    # must return a stalled XR session to the usable launch page.
    fresh();launch()
    page.evaluate('''() => { const now=performance.now.bind(performance);
        performance.now=()=>now()+13000; }''')
    page.wait_for_function("!document.getElementById('enter').disabled",timeout=10000)
    assert 'stopped receiving frames' in page.locator('#status').inner_text()
    report=json.loads(page.locator('#xr-report').inner_text())
    assert report['error'] and report['eyes']==0 and page.locator('#diagnostics').get_attribute('open') is not None
    assert state(page)['paused'] and not xr_state(page)['active']
    assert not page.locator('#export').is_disabled()

    fresh();page.evaluate("xrTest.rightPad.gamepad.mapping=''")
    launch();step(page,2)
    assert state(page)['paused'] and xr_state(page)['eyes']>0
    page.evaluate('''() => { const now=performance.now.bind(performance);
        performance.now=()=>now()+21000; xrTest.step(); }''')
    page.wait_for_function("!document.getElementById('enter').disabled",timeout=10000)
    assert 'could not finish tracking' in page.locator('#status').inner_text()
    report=json.loads(page.locator('#xr-report').inner_text())
    assert any(c['hand']=='right' and c['mapping']=='' for c in report['controllers'])
    assert report['eyes']>0 and state(page)['paused'] and not xr_state(page)['active']
    (ARTIFACTS/'startup-diagnostics.json').write_text(json.dumps(report,indent=2)+'\n')
    fresh()
    return 'head-only startup/recovery images, delayed/null/emulated controller poses, vertical entry, deliberate input recovery, stalled-session timeouts and diagnostic report'


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=ROOT/'build-pages-preview')
    parser.add_argument('--url-path', default='/starship_crawl/quest/')
    parser.add_argument('--ordinary-path', default='/starship_crawl/')
    parser.add_argument('--journeys-only', action='store_true', help='Focus on imported progression/facility journeys during development')
    parser.add_argument('--startup-only', action='store_true', help='Focus on headset startup and recovery regressions')
    parser.add_argument('--intro-only', action='store_true', help='Focus on the earned introductory matrix link')
    parser.add_argument('--projection-targets', action='store_true', help='Exercise gameplay through granted projection textures')
    args = parser.parse_args()
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer(('127.0.0.1',0),partial(QuietHandler,directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    origin=f'http://127.0.0.1:{server.server_port}'
    url=origin+args.url_path
    results=[]
    page=None
    pw=None
    try:
        pw=sync_playwright().start()
        options={'headless':True,'args':['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']}
        if os.environ.get('SC_BROWSER'): options['executable_path']=os.environ['SC_BROWSER']
        browser=pw.chromium.launch(**options)
        context=browser.new_context(viewport={'width':1280,'height':900},accept_downloads=True)
        page=context.new_page()
        errors=[]
        page.on('pageerror',lambda error:errors.append(str(error)))
        page.on('console',lambda msg: print('BROWSER',msg.text,flush=True) if msg.type=='error' else None)
        device=(ROOT/'tests/quest_device.js').read_text()
        if args.projection_targets:
            device+='\nxrTest.projectionSupported=true;\n'
        page.add_init_script(script=device)
        page.goto(url+'?seed=713');ready(page)
        if not args.journeys_only and not args.intro_only:
            results.append(projection_checks(page,url))
            print('PASS:',results[-1],flush=True)
            results.append(framebuffer_checks(page,url))
            print('PASS:',results[-1],flush=True)
            results.append(startup_checks(page,url))
            print('PASS:',results[-1],flush=True)
            if args.startup_only:
                assert not errors,errors
                (ARTIFACTS/'startup-results.json').write_text(json.dumps({'checks':results,'physical_headset_tested':False},indent=2)+'\n')
                browser.close()
                return
            assert state(page)['paused']
            page.evaluate('xrTest.denied=true')
            page.locator('#enter').click()
            page.wait_for_function("document.getElementById('status').textContent.includes('Permission denied')")
            assert state(page)['paused'] and not xr_state(page)['active']
            page.evaluate('xrTest.denied=false;xrTest.failLayer=true')
            page.locator('#enter').click()
            page.wait_for_function("document.getElementById('status').textContent.includes('layer setup rejected')")
            assert not xr_state(page)['active']
            page.evaluate('xrTest.failLayer=false;xrTest.localFloor=false')
            enter(page)
            if args.projection_targets:
                assert page.evaluate('xrTest.layer.projection===true')
            q=xr_state(page); assert q['eyes']==2*q['frames'],q
            assert page.evaluate('xrTest.session.rate')==72
            capture(page,'quest-approach-stereo')
            results.append('session permission/setup failure, local reference fallback, stereo views and single XR frame loop')

            before=state(page)
            page.evaluate('xrTest.rightAim.yaw=-Math.PI/2')
            press(page,'right',0)
            after=state(page)
            assert after['shots']>before['shots'] and abs(after['yaw']-before['yaw'])<.001
            assert xr_state(page)['aim'][0]>.99
            page.evaluate('xrTest.rightAim.yaw=0')
            page.evaluate("xrTest.stick('right',1,0)");step(page,5)
            snapped=state(page)['yaw'];step(page,5)
            assert abs(snapped-before['yaw']-3.141592653589793/4)<.001
            assert state(page)['yaw']==snapped
            page.evaluate("xrTest.stick('right',0,0)");step(page)
            page.evaluate("xrTest.stick('right',-1,0)");step(page)
            page.evaluate("xrTest.stick('right',0,0)");step(page)
            assert abs(state(page)['yaw'])<.001
            results.append('independent controller aim, held-stick snap latch and neutral release')

            press(page,'left',5)
            before=state(page)
            step(page,5)
            assert state(page)['time']==before['time']
            page.evaluate('xrTest.head.yaw=.15');step(page)
            assert state(page)['yaw']!=before['yaw'] and state(page)['time']==before['time']
            capture(page,'quest-floating-menu-stereo')
            labels = []
            for _ in range(60):
                labels.append(xr_state(page)['label'])
                page.evaluate("xrTest.stick('right',0,1)"); step(page)
                page.evaluate("xrTest.stick('right',0,0)"); step(page)
                if xr_state(page)['selected'] == 0: break
            else: raise AssertionError(('menu did not wrap', labels))
            assert not any('MATRIX PROGRAMS' in label or 'ABILITIES AND EQUIPMENT' in label for label in labels), labels
            choose(page,'GUIDANCE: GUIDED')
            assert state(page)['guidance'] == 1
            choose(page,'GUIDANCE: MINIMAL')
            assert state(page)['guidance'] == 2
            choose(page,'FIELD GUIDE / REVIEW')
            capture(page,'quest-onboarding-guide')
            press(page,'right',5)
            choose(page,'GUIDANCE: OFF')
            assert state(page)['guidance'] == 0
            results.append('earned-only Quest menus, optional guidance and reviewable controller field guide')
            page.evaluate('xrTest.head.yaw=0');step(page)
            choose(page,'MAP AND OBJECTIVE')
            assert state(page)['map']
            capture(page,'quest-objective-map-stereo')
            press(page,'right',5)
            choose(page,'MOVEMENT VIGNETTE')
            assert not xr_state(page)['vignette']
            choose(page,'RESUME EXPEDITION')
            page.evaluate("Object.assign(xrTest.left,{x:-.1,y:1.4,z:-.45});Object.assign(xrTest.leftAim,xrTest.left);xrTest.button('left',1,true)");step(page,20)
            capture(page,'quest-wrist-map-stereo')
            page.evaluate('xrTest.neutral()');step(page)
            results.append('paused head tracking, controller menus, objective map, wrist map, persisted comfort option')

            page.evaluate("xrTest.button('right',0,true);xrTest.stick('left',0,-1)");step(page,2)
            page.evaluate('xrTest.rightAvailable=false');step(page)
            frozen=state(page)
            step(page,3,60000)
            assert state(page)['time']==frozen['time'] and state(page)['shots']==frozen['shots']
            page.evaluate('xrTest.rightAvailable=true');step(page,3)
            assert state(page)['paused']
            page.evaluate('xrTest.neutral()');step(page)
            assert state(page)['paused']
            press(page,'right',4)
            assert not state(page)['paused'] and state(page)['shots']==frozen['shots']
            assert state(page)['time']-frozen['time']<.1
            page.evaluate("xrTest.session.visibilityState='hidden';xrTest.session.dispatchEvent(new Event('visibilitychange'))")
            frozen=state(page);step(page,2,60000)
            assert state(page)['time']==frozen['time']
            page.evaluate("xrTest.session.visibilityState='visible'");step(page)
            assert state(page)['paused'];press(page,'right',4)
            page.evaluate('xrTest.headAvailable=false');step(page)
            assert state(page)['paused']
            page.evaluate('xrTest.headAvailable=true');step(page);press(page,'right',4)
            page.evaluate('xrTest.head.x=2');step(page)
            assert xr_state(page)['unsafe']
            capture(page,'quest-lean-recovery')
            page.evaluate('xrTest.head.x=0');step(page)
            assert not xr_state(page)['unsafe']
            results.append('controller loss, held-fire rejection, visibility/head tracking loss, bounded leaning, deliberate recovery with no time catch-up')

            # Travel from the genuine fresh arrival to the loading bay.
            page.evaluate("xrTest.stick('left',0,-1)")
            for _ in range(60):
                step(page,10,50)
                if state(page)['z']>4: break
            page.evaluate('xrTest.neutral()');step(page,8)
            assert state(page)['z']>4,state(page)
            capture(page,'quest-loading-bay-stereo')
            results.append('fresh-character approach and boarding using controller locomotion')
            page.evaluate('xrTest.head.pitch=Math.PI/2');step(page)
            assert abs(state(page)['pitch'])<=1.5
            page.evaluate('scQuest.end()')
            page.wait_for_function("!document.getElementById('enter').disabled")
            assert state(page)['paused']
            page.wait_for_function('scStorage.status===3')
            saved=state(page)
            page.evaluate('xrTest.head.pitch=0')
            with page.expect_download() as download:
                page.locator('#export').click()
            download.value.save_as(ARTIFACTS/'quest-export.sav')
            page.goto(url);ready(page)
            assert abs(state(page)['z']-saved['z'])<.01
            assert not page.locator('#vignette').is_checked()
            enter(page)
            assert xr_state(page)['eyes']==2*xr_state(page)['frames']
            page.evaluate('scQuest.end()');page.wait_for_function("!document.getElementById('export').disabled")
            page.locator('#import').set_input_files({'name':'invalid.sav','mimeType':'application/octet-stream','buffer':b'invalid'})
            page.wait_for_function("document.getElementById('status').textContent.includes('retained')")
            assert state(page)['seed']==713
            results.append('session end/re-entry, saved position reload, export and invalid-import rollback')

            # Ordinary and VR pages share an origin but must not share databases.
            ordinary=context.new_page()
            ordinary.goto(origin+args.ordinary_path+'?seed=42')
            ordinary.wait_for_function("!document.getElementById('start').disabled",timeout=90000)
            ordinary.evaluate("Module.ccall('sc_web_save',null,[],[])")
            ordinary.wait_for_function('scStorage.status===3',timeout=30000)
            assert state(ordinary)['seed']==42
            page.reload();ready(page)
            assert state(page)['seed']==713
            ordinary.goto(origin+args.ordinary_path);ordinary.wait_for_function("!document.getElementById('start').disabled",timeout=90000)
            assert state(ordinary)['seed']==42
            ordinary.close()
        import_save(page,'onboarding-interface.sav');enter(page)
        assert state(page)['intro']==1 and state(page)['programs']==1
        press(page,'right',1)
        assert state(page)['matrix']
        capture(page,'quest-onboarding-first-link')
        press(page,'right',1)
        restored=state(page)
        assert not restored['matrix'] and restored['intro']==3 and restored['programs']==7 and restored['systems']&1,restored
        capture(page,'quest-onboarding-restored-power')
        page.evaluate('scQuest.end()');page.wait_for_function("!document.getElementById('export').disabled")
        results.append('earned interface first link, protected power restoration and physical return with Quest grips')
        if args.intro_only:
            assert not errors,errors
            (ARTIFACTS/'intro-results.json').write_text(json.dumps({'checks':results,'physical_headset_tested':False},indent=2)+'\n')
            print('PASS:',results[-1],flush=True)
            browser.close()
            return
        page.locator('#import').set_input_files(str(ROOT/'tests/fixtures/phase-reward.sav'))
        page.wait_for_function("document.getElementById('status').textContent.includes('Save imported')")
        enter(page);press(page,'left',5);choose(page,'PHASES AND LOADOUT')
        capture(page,'quest-earned-progression-stereo')
        reward=state(page)
        assert reward['reward_pending']>0
        choose(page,reward['offer1'])
        assert state(page)['claimed']!=reward['claimed'] and state(page)['reward_pending']==0
        page.evaluate('scQuest.end()');page.wait_for_function("!document.getElementById('export').disabled")
        import_save(page,'phase-unbound.sav');enter(page)
        press(page,'left',5);choose(page,'CHARACTER UPGRADES')
        old=state(page)
        assert old['points']>0
        press(page,'right',4)
        assert state(page)['points']==old['points']-1
        page.evaluate('scQuest.end()');page.wait_for_function("!document.getElementById('export').disabled")
        import_save(page,'phase-decision.sav');enter(page)
        press(page,'right',1)
        assert state(page)['decision'] and state(page)['paused'],state(page)
        story=state(page)
        choose(page,'1: ')
        assert not state(page)['decision'] and state(page)['story_done']!=story['story_done']
        capture(page,'quest-story-decision-stereo')
        page.evaluate('scQuest.end()');page.wait_for_function("!document.getElementById('export').disabled")
        import_save(page,'generator-v6.sav')
        enter(page)
        assert state(page)['sheltered']
        capture(page,'quest-shelter-stereo')
        walk(page,3,-5,sheltered=True)
        press(page,'left',5);choose(page,'MATRIX PROGRAMS');choose(page,'CONNECT AT TERMINAL')
        assert state(page)['matrix']
        press(page,'left',5);step(page,20)
        before=state(page)['shots'];press(page,'right',0)
        assert state(page)['shots']>before
        capture(page,'quest-matrix-stereo')
        press(page,'left',5);choose(page,'MATRIX PROGRAMS');choose(page,'DISCONNECT / RETURN')
        assert not state(page)['matrix'] and state(page)['sheltered']
        press(page,'left',5)
        walk(page,0,-6.5,sheltered=True)
        press(page,'right',1)
        assert not state(page)['sheltered'] and xr_state(page)['fade']>0
        # Legacy sandbox capabilities are imported normally, never granted by the test.
        for rig,name in [(1,'MECH'),(2,'WAGON')]:
            press(page,'left',5);choose(page,'ABILITIES AND EQUIPMENT');choose(page,'DEPLOY / FOLD '+name)
            press(page,'left',5);press(page,'right',1)
            assert state(page)['riding']==rig,state(page)
            assert xr_state(page)['fade']>0
            page.evaluate('xrTest.button("left",1,true)');step(page,20)
            capture(page,'quest-'+name.lower()+'-stereo')
            page.evaluate('xrTest.neutral()');step(page)
            press(page,'right',1)
            assert state(page)['riding']==0
            press(page,'left',5);choose(page,'ABILITIES AND EQUIPMENT');choose(page,'DEPLOY / FOLD '+name)
            press(page,'left',5)
        page.evaluate('scQuest.end()');page.wait_for_function("!document.getElementById('export').disabled")
        results.append('earned reward claim, upgrade and story decision; shelter, matrix aiming/return, mech and wagon transitions through controller actions')
        page.locator('#import').set_input_files(str(ROOT/'tests/fixtures/generator-v1.sav'))
        page.wait_for_function("document.getElementById('status').textContent.includes('Save imported')")
        results.append('earned-current and legacy save imports' if args.journeys_only else 'same-origin ordinary/Quest storage isolation and earned-current/legacy save imports')

        enter(page)
        page.evaluate("xrTest.layer.gl.getExtension('WEBGL_lose_context').loseContext()")
        page.wait_for_function("!document.getElementById('reload').hidden")
        assert state(page)['paused'] and page.locator('#enter').is_disabled()
        page.reload();ready(page)
        unsupported=context.new_page()
        unsupported.add_init_script((ROOT/'tests/quest_device.js').read_text()+'\nxrTest.supported=false;')
        unsupported.goto(url);ready(unsupported)
        unsupported.wait_for_function("document.getElementById('status').textContent.includes('unavailable')")
        assert unsupported.locator('#enter').is_disabled()
        assert not unsupported.locator('#export').is_disabled()
        unsupported.close()
        results.append('graphics-loss reload path and usable unsupported-browser launch page')
        assert not errors,errors
        browser.close()
        (ARTIFACTS/'results.json').write_text(json.dumps({'scope':'journeys' if args.journeys_only else 'full',
            'render_target_preference':'projection-textures' if args.projection_targets else 'base-layer',
            'checks':results,'physical_headset_tested':False},indent=2)+'\n')
        for result in results:print('PASS:',result,flush=True)
    except Exception:
        try:
            if page and not page.is_closed():
                snapshot={'game':state(page),'xr':xr_state(page),'page_status':page.locator('#status').inner_text()}
                (ARTIFACTS/'failure-state.json').write_text(json.dumps(snapshot,indent=2)+'\n')
                print('FAILURE',snapshot['xr'],snapshot['page_status'],flush=True)
                page.screenshot(path=str(ARTIFACTS/'failure-launch.png'))
        except Exception:
            pass
        raise
    finally:
        if pw: pw.stop()
        server.shutdown();server.server_close()


if __name__=='__main__':run()
