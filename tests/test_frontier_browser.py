"""Real keyboard/mouse/touch checks of packaged deep-frontier scenes and earned saves."""
import argparse
import json
import math
import os
from functools import partial
from http.server import ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_ready, wait_for
from test_interiors_browser import expect
from test_regional_browser import aim_at
from test_habitat_browser import capture
from test_story_browser import import_fixture


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
            browser = pw.chromium.launch(headless=True, executable_path=os.environ.get('SC_BROWSER', '/usr/bin/chromium'),
                args=['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
            context = browser.new_context(viewport={'width':1280, 'height':720})
            page = context.new_page(); errors=[]
            page.on('pageerror', lambda e: errors.append(str(e)))
            page.on('dialog',lambda d:d.accept())
            hashes=set()
            for reveal in range(1,10):
                page.goto(base+f'?seed=713&scene=reveal{reveal}');wait_ready(page);start_play(page)
                s=state(page);assert s['generator']==12 and s['reveal']==reveal and not s['failed'], s
                pixels=capture(page,f'frontier-reveal{reveal}')
                assert pixels['lit']>80 and pixels['colored']>20,(reveal,pixels)
                hashes.add(pixels['sum'])
            assert len(hashes)==9
            print('PASS nine generated focal structures and rendered scale cues',flush=True)
            for kind in range(6):
                page.goto(base+f'?seed=713&scene=front{kind}');wait_ready(page);start_play(page)
                s=state(page);assert s['challenge']==kind and s['frontier_living']==4 and not s['frontier_active'],s
                page.screenshot(path=str(ARTIFACTS/f'frontier-{kind}-preview.png'))
                page.keyboard.press('g');expect(page,'s.frontier_active')
                expect(page,'(s.alien_motions & 8) !== 0')
                page.screenshot(path=str(ARTIFACTS/f'frontier-{kind}-active.png'))
                if kind==1:assert max(e['at'][1] for e in state(page)['regional_enemies']) > s['y']+5
                if kind==4:
                    cursor=[640,360];page.mouse.move(*cursor)
                    # Both pylons are reachable on foot; no vehicle key gates this fight.
                    for sign in [-1,1]:
                        now=state(page);target=[s['x']+28*sign,s['y']+1.6,s['z']]
                        aim_at(page,target,cursor)
                        page.keyboard.down('Shift');page.keyboard.down('w')
                        try:expect(page,f"(s.circuit & {1 if sign<0 else 2}) !== 0",timeout=30000)
                        finally:page.keyboard.up('w');page.keyboard.up('Shift')
                    assert state(page)['circuit']==3
                page.keyboard.press('Escape');expect(page,'s.paused')
                paused=state(page);expect(page,f"s.frames > {paused['frames']+5}")
                assert state(page)['time']==paused['time'] and state(page)['specialists']==paused['specialists']
            print('PASS all encounter previews, activation, warnings, foot circuit and paused simulation',flush=True)
            for contract in range(5):
                page.goto(base+f'?seed=713&scene=deep{contract}');wait_ready(page);start_play(page)
                assert state(page)['contract']==contract+1
                page.keyboard.press('g');expect(page,'s.frontier_active && s.hazard_time > 1.4 && s.hazard_time < 2.6')
                page.screenshot(path=str(ARTIFACTS/f'frontier-contract-{contract}.png'))
            for kind in range(10):
                page.goto(base+f'?seed=713&scene=alien{kind}');wait_ready(page);start_play(page)
                s=state(page);assert len(s['specialists'])==1 and s['specialists'][0]['kind']==32+kind,s
                expect(page,f"s.time > {s['time']+.6}")
                page.screenshot(path=str(ARTIFACTS/f'frontier-alien-{kind}.png'))
                if kind==0:
                    page.keyboard.down('Control');page.keyboard.down('w');before=state(page)
                    try:expect(page,f"s.time > {before['time']+.5}")
                    finally:page.keyboard.up('w');page.keyboard.up('Control')
                    assert state(page)['specialists'][0]['motion'] not in [3,4]
                    page.mouse.down();before=state(page)
                    expect(page,f"s.time > {before['time']+.1}");page.mouse.up()
                    expect(page,'s.specialists[0].sound > 0 || !s.specialists[0].alive')
                if kind==1:
                    before=state(page)['specialists'][0];assert before['observed']
                    expect(page,f"s.time > {s['time']+1.2}")
                    assert state(page)['specialists'][0]['at']==before['at']
                    now=state(page);cursor=[640,360];page.mouse.move(*cursor)
                    aim_at(page,[now['x'],now['y']+1.6,now['z']-15],cursor)
                    expect(page,'!s.specialists[0].observed')
                    expect(page,f"s.specialists[0].at[2] < {before['at'][2]-.2}")
                if kind==4:
                    cursor=[640,360];page.mouse.move(*cursor)
                    for _ in range(35):
                        s=state(page)
                        if s['specialists'][0]['molted']:break
                        aim_at(page,s['specialists'][0]['at'],cursor)
                        page.mouse.down();expect(page,f"s.time > {s['time']+.2}");page.mouse.up()
                    assert state(page)['specialists'][0]['molted']
                    page.screenshot(path=str(ARTIFACTS/'frontier-molt.png'))
            print('PASS ten rendered species, quiet approach, observation freeze and actual molt combat',flush=True)
            # Imported checkpoint was earned by the ordinary gameplay driver.
            page.goto(base+'?seed=713');wait_ready(page);start_play(page)
            import_fixture(page,'frontier-1.sav')
            before=state(page)
            assert before['generator']==12 and before['challenge']==1 and not before['frontier_active'],before
            page.keyboard.press('g');expect(page,'s.frontier_active')
            cursor=[640,360];page.mouse.move(*cursor)
            for _ in range(150):
                s=state(page)
                if s['local_state'] & 2:break
                alive=[e for e in s['regional_enemies'] if e['alive']]
                assert alive,s
                aim_at(page,alive[0]['at'],cursor)
                page.mouse.down();expect(page,f"s.time > {s['time']+.18}");page.mouse.up()
            expect(page,'(s.local_state & 2) !== 0')
            page.keyboard.press('g');expect(page,'(s.local_state & 16) !== 0')
            earned=state(page)
            assert sum(earned['relics'])==sum(before['relics'])+1,(before,earned)
            page.keyboard.press('g')
            assert state(page)['relics']==earned['relics']
            page.keyboard.press('F10')
            page.screenshot(path=str(ARTIFACTS/'frontier-relic-journal.png'))
            page.keyboard.press('Enter');expect(page,'!s.paused')
            saving_at=state(page)['time'];page.keyboard.press('F5')
            expect(page,f"s.time > {saving_at+.12}")
            wait_for(page,'window.scStorage.status === 3')
            page.goto(base);wait_ready(page)
            assert state(page)['relics']==earned['relics'] and state(page)['local_state'] & 16
            assert state(page)['frontier_living']==0
            (ARTIFACTS/'frontier-earned.json').write_text(json.dumps(state(page),indent=2))
            print('PASS earned combat, relic evolution, once-only claim, journal and IndexedDB resume',flush=True)
            page.goto(base+'?seed=713&scene=deep0');wait_ready(page);start_play(page)
            page.keyboard.press('F2');expect(page,'s.players === 2')
            page.keyboard.press('g');expect(page,'s.frontier_active && (s.local_state & 12) === 12')
            page.screenshot(path=str(ARTIFACTS/'frontier-split.png'))
            assert not errors,errors
            context.close()
            mobile=browser.new_context(viewport={'width':844,'height':390},is_mobile=True,has_touch=True)
            page=mobile.new_page();page.on('pageerror',lambda e:errors.append(str(e)))
            page.goto(base+'?seed=713&scene=deep2');wait_ready(page);start_play(page)
            # Real touch USE, through the same production touch event path.
            client=mobile.new_cdp_session(page)
            client.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':760,'y':105,'id':1}]})
            client.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
            expect(page,'s.frontier_active')
            page.screenshot(path=str(ARTIFACTS/'frontier-touch.png'))
            page.set_viewport_size({'width':390,'height':844})
            page.screenshot(path=str(ARTIFACTS/'frontier-portrait.png'))
            assert not state(page)['failed'] and not errors,errors
            mobile.close();browser.close()
    finally:
        server.shutdown();server.server_close()
    print('PASS deep frontiers WebGL; physical-device and human play remain separate',flush=True)

if __name__=='__main__':run()
