"""Exercise device preferences, input ownership and lifecycle through the real web app."""
from functools import partial
from http.server import ThreadingHTTPServer
from pathlib import Path
import argparse
import json
import os
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ROOT, BUILD, ARTIFACTS, QuietHandler, state, wait_ready, wait_for, start_play


def controls(page):
    return json.loads(page.evaluate("Module.ccall('sc_web_preferences','string',[],[])"))


def expect_controls(page, expression):
    # Software WebGL can advance a multi-second lift ride slowly under CPU load.
    # Keep the actual transit/deck/latch assertions; this is a wall-time allowance.
    wait_for(page, "() => {const c=JSON.parse(Module.ccall('sc_web_preferences','string',[],[])); return " + expression + ";}", timeout=60000)


VIRTUAL_PADS = """
window.virtualPads=[null,null,null,null];
Object.defineProperty(navigator,'getGamepads',{value:()=>window.virtualPads});
window.connectVirtualPad=index=>{
  const pad={id:'Starship virtual standard pad '+index,index,connected:true,mapping:'standard',
    axes:[0,0,0,0],buttons:Array.from({length:17},()=>({pressed:false,touched:false,value:0})),
    get timestamp(){return performance.now()}};
  window.virtualPads[index]=pad;
  const event=new Event('gamepadconnected');Object.defineProperty(event,'gamepad',{value:pad});window.dispatchEvent(event);
};
window.disconnectVirtualPad=index=>{
  const pad=window.virtualPads[index];pad.connected=false;window.virtualPads[index]=null;
  const event=new Event('gamepaddisconnected');Object.defineProperty(event,'gamepad',{value:pad});window.dispatchEvent(event);
};
"""


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir', type=Path, default=BUILD)
    parser.add_argument('--url-path', default='/starship_crawl.html')
    args = parser.parse_args()
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(QuietHandler, directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    base = f'http://127.0.0.1:{server.server_port}{args.url_path}'
    try:
        with sync_playwright() as pw:
            options = {'headless': True, 'args': ['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader']}
            if os.environ.get('SC_BROWSER'):
                options['executable_path'] = os.environ['SC_BROWSER']
            else:
                options['channel'] = 'chromium'
            browser = pw.chromium.launch(**options)
            context = browser.new_context(viewport={'width': 1280, 'height': 720}, accept_downloads=True)
            context.add_init_script(VIRTUAL_PADS)
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=713')
            wait_ready(page)
            start_play(page)
            if not state(page)['paused']:
                page.keyboard.press('Escape')
            wait_for(page, 'document.pointerLockElement===null')
            page.locator('#menu').click()
            page.locator('#preferences summary').click()
            page.locator('#pref-mouse').focus()
            page.keyboard.press('ArrowRight')
            page.locator('#pref-invert').check()
            page.locator('#pref-quality').select_option('1')
            expect_controls(page, 'c.views[0][0]===1.25 && c.views[0][4]===1 && c.height[0]===180')
            page.locator('#pref-player').select_option('1')
            page.locator('#pref-quality').select_option('4')
            expect_controls(page, 'c.views[1][5]===4 && c.views[0][5]===1')
            wait_for(page, 'window.scStorage.status===3')
            page.screenshot(path=str(ARTIFACTS / 'foundation-preferences.png'))
            with page.expect_download() as pending:
                page.locator('#export').click()
            saved = pending.value.path().read_bytes()
            page.reload()
            wait_ready(page)
            assert controls(page)['views'][0] == [1.25, 1, 1, .16, 1, 1, 0]
            assert controls(page)['views'][1][5] == 4
            start_play(page)
            if not state(page)['paused']:
                page.keyboard.press('Escape')
            wait_for(page, 'document.pointerLockElement===null')
            page.locator('#menu').click()
            page.locator('#new').click()
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).seed!==713")
            assert controls(page)['views'][0][0] == 1.25
            if not state(page)['paused']:
                page.keyboard.press('Escape')
            wait_for(page, 'document.pointerLockElement===null')
            page.locator('#menu').click()
            page.locator('#import').set_input_files({'name': 'expedition.sav', 'mimeType': 'application/octet-stream', 'buffer': saved})
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).seed===713")
            assert controls(page)['views'][0][0] == 1.25
            page.keyboard.press('F1')
            expect_controls(page, 'c.options')
            page.screenshot(path=str(ARTIFACTS / 'foundation-native-options.png'))
            page.keyboard.press('Enter')
            page.keyboard.press('F2')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).players===2")
            expect_controls(page, 'c.height[0]===180 && c.height[1]===480')
            before = controls(page)['resizes']
            page.wait_for_timeout(600)
            assert controls(page)['resizes'] == before, 'Per-view targets must be cached, not reallocated every frame'
            page.screenshot(path=str(ARTIFACTS / 'foundation-independent-quality.png'))
            page.keyboard.press('F2')
            page.evaluate('connectVirtualPad(0);connectVirtualPad(1)')
            expect_controls(page, 'c.pads[0]===0 && c.pads[1]===0')
            page.keyboard.press('F2')
            expect_controls(page, 'c.pads[0]===0 && c.pads[1]===1')
            before_yaw = state(page)['yaw']
            before_p2 = controls(page)['p2'][3]
            page.evaluate('virtualPads[0].axes[2]=.7;virtualPads[1].axes[2]=-.7')
            wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).yaw>{before_yaw+.1}")
            expect_controls(page, f'c.p2[3]<{before_p2-.1}')
            page.evaluate('virtualPads[0].axes[2]=0;virtualPads[1].axes[2]=0;disconnectVirtualPad(1)')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            expect_controls(page, 'c.pads[0]===0 && c.pads[1]===-1')
            before_p2 = controls(page)['p2'][3]
            page.keyboard.press('Enter')
            page.evaluate('virtualPads[0].axes[2]=.7')
            before_yaw = state(page)['yaw']
            wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).yaw>{before_yaw+.1}")
            assert abs(controls(page)['p2'][3] - before_p2) < .01
            page.evaluate('virtualPads[0].axes[2]=0;connectVirtualPad(1);disconnectVirtualPad(0)')
            expect_controls(page, 'c.pads[0]===-1 && c.pads[1]===1')
            page.keyboard.press('Enter')
            before_yaw = state(page)['yaw']
            before_p2 = controls(page)['p2'][3]
            page.evaluate('virtualPads[1].axes[2]=.7')
            expect_controls(page, f'c.p2[3]>{before_p2+.1}')
            assert abs(state(page)['yaw'] - before_yaw) < .01
            page.evaluate('virtualPads[1].axes[2]=0')
            page.keyboard.down('w')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).key_w===1")
            # Headless Chromium keeps background pages visible/focused. Deliver the
            # browser lifecycle event at the DOM boundary; no C game state is changed.
            page.evaluate("window.dispatchEvent(new FocusEvent('blur'))")
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            frozen = state(page)
            page.wait_for_timeout(300)
            assert state(page)['time'] == frozen['time'] and state(page)['key_w'] == 0
            page.keyboard.up('w')
            page.keyboard.press('Enter')
            page.set_viewport_size({'width': 1, 'height': 1})
            page.wait_for_timeout(150)
            page.set_viewport_size({'width': 720, 'height': 960})
            page.wait_for_timeout(300)
            assert not state(page)['failed'] and not errors, errors
            print('PASS foundation desktop: persisted settings, import/new-ship independence, cached split targets, gamepad ownership and focus', flush=True)
            context.close()
            # Real keyboard traversal in production inspection scenes, with no position setters.
            scene_context = browser.new_context(viewport={'width': 1280, 'height': 720})
            page = scene_context.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            for scene in ['stairs', 'ramp']:
                page.goto(base + '?seed=713&scene=' + scene)
                wait_ready(page)
                start_play(page)
                assert state(page)['generator'] == 15
                page.screenshot(path=str(ARTIFACTS / ('foundation-' + scene + '.png')))
                page.keyboard.down('w')
                wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).y>4", timeout=20000)
                page.keyboard.up('w')
                page.keyboard.press('Escape')
                assert 4 < state(page)['y'] < 9 and not controls(page)['transit']
                page.screenshot(path=str(ARTIFACTS / ('foundation-' + scene + '-climb.png')))
            page.goto(base + '?seed=713')
            wait_ready(page)
            start_play(page)
            page.keyboard.down('w')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).z>37", timeout=90000)
            page.keyboard.up('w')
            page.keyboard.press('F2')
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).players===2")
            page.keyboard.down('e')
            expect_controls(page, 'c.transit && c.progress>.25 && c.progress<.7')
            page.keyboard.up('e')
            assert 5 < state(page)['y'] < 59 and abs(controls(page)['p2'][1]) < .05
            page.screenshot(path=str(ARTIFACTS / 'foundation-lift-transit.png'))
            page.keyboard.press('Escape')
            assert controls(page)['transit']
            page.evaluate("Module.ccall('sc_web_save',null,[],[])")
            wait_for(page, 'window.scStorage.status===3')
            page.goto(base)
            wait_ready(page)
            assert state(page)['seed']==713 and abs(state(page)['z']-38)<.05
            assert not controls(page)['transit'] and abs(state(page)['y']) < .05
            start_play(page)
            page.keyboard.down('e')
            expect_controls(page, 'c.transit')
            page.keyboard.up('e')
            expect_controls(page, '!c.transit && c.progress===1')
            assert abs(state(page)['y'] - 64.02) < .05
            page.keyboard.down('q')
            expect_controls(page, 'c.transit && c.deck===0')
            page.keyboard.up('q')
            expect_controls(page, '!c.transit && c.progress===1')
            assert abs(state(page)['y']) < .05 and not errors, errors
            scene_context.close()
            print('PASS foundation traversal: actual keyboard stair/ramp ascent and split-screen lift, safe IndexedDB ride snapshot and round trip', flush=True)
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, is_mobile=True, has_touch=True)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(base + '?seed=713')
            wait_ready(page)
            page.locator('#start').tap()
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            client = mobile.new_cdp_session(page)
            client.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': [{'id': 1, 'x': 110, 'y': 280}, {'id': 2, 'x': 760, 'y': 280}]})
            client.send('Input.dispatchTouchEvent', {'type': 'touchMove', 'touchPoints': [{'id': 1, 'x': 110, 'y': 230}, {'id': 2, 'x': 760, 'y': 280}]})
            wait_for(page, "JSON.parse(Module.ccall('sc_web_state','string',[],[])).shots>0")
            page.set_viewport_size({'width': 390, 'height': 844})
            page.wait_for_timeout(300)
            before = state(page)
            page.wait_for_timeout(400)
            after = state(page)
            assert abs(before['z'] - after['z']) < .02 and before['shots'] == after['shots'], (before, after)
            client.send('Input.dispatchTouchEvent', {'type': 'touchCancel', 'touchPoints': []})
            page.set_viewport_size({'width': 844, 'height': 390})
            page.wait_for_timeout(200)
            client.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': [{'id': 3, 'x': 760, 'y': 280}]})
            wait_for(page, f"JSON.parse(Module.ccall('sc_web_state','string',[],[])).shots>{after['shots']}")
            client.send('Input.dispatchTouchEvent', {'type': 'touchCancel', 'touchPoints': []})
            page.wait_for_timeout(200)
            before = state(page)['shots']
            page.wait_for_timeout(300)
            assert state(page)['shots'] == before
            page.locator('#menu').tap()
            page.locator('#preferences summary').tap()
            page.locator('#pref-quality').select_option('1')
            expect_controls(page, 'c.height[0]===180')
            page.screenshot(path=str(ARTIFACTS / 'foundation-touch-options.png'))
            assert not state(page)['failed'] and not errors, errors
            page.goto(base + '?seed=713&scene=lift')
            wait_ready(page)
            page.locator('#start').tap()
            wait_for(page, "!JSON.parse(Module.ccall('sc_web_state','string',[],[])).paused")
            client.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': [{'id': 4, 'x': 760, 'y': 187}]})
            expect_controls(page, 'c.transit && c.deck===1')
            client.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})
            expect_controls(page, '!c.transit && c.progress===1')
            assert abs(state(page)['y'] - 64.02) < .05
            mobile.close()
            browser.close()
            print('PASS foundation mobile: rotation/cancellation clear held touches and fresh input/settings and touch lifts remain usable', flush=True)
    except Exception:
        print('::error title=Foundation browser regression::' + traceback.format_exc().replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A'), flush=True)
        raise
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
