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


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=ROOT/'build-pages-preview')
    parser.add_argument('--url-path', default='/starship_crawl/quest/')
    parser.add_argument('--ordinary-path', default='/starship_crawl/')
    parser.add_argument('--journeys-only', action='store_true', help='Focus on imported progression/facility journeys during development')
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
        page.add_init_script(path=str(ROOT/'tests/quest_device.js'))
        page.goto(url+'?seed=713');ready(page)
        if not args.journeys_only:
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
        (ARTIFACTS/'results.json').write_text(json.dumps({'scope':'journeys' if args.journeys_only else 'full', 'checks':results,'physical_headset_tested':False},indent=2)+'\n')
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
