"""Court audiences, actual earned finales, co-op, touch and saved political state."""
import argparse
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
            if os.environ.get('SC_CPU_THROTTLE'):
                context.new_cdp_session(page).send('Emulation.setCPUThrottlingRate', {'rate': float(os.environ['SC_CPU_THROTTLE'])})
            page.on('pageerror', lambda e: errors.append(str(e)))
            page.on('dialog', lambda d: d.accept())
            hashes=set()
            for nation in range(3):
                page.goto(base+f'?seed=713&scene=court{nation}'); wait_ready(page); start_play(page)
                s=state(page); assert s['generator']==13 and s['court']==1 and s['frontier_living']==0,s
                pixels=capture(page,f'court-{nation}'); assert pixels['lit']>80 and pixels['colored']>20,pixels
                hashes.add(pixels['sum'])
                page.keyboard.press('g');expect(page,f's.paused && s.journal_page===6 && s.audience==={nation+1}')
                page.screenshot(path=str(ARTIFACTS/f'court-{nation}-audience.png'))
                paused=state(page);expect(page,f"s.frames>{paused['frames']+5}")
                assert state(page)['director_cooldown']==paused['director_cooldown']
                page.keyboard.press('Enter');expect(page,'!s.paused')
                assert state(page)['court_stages'][nation]==0
                page.keyboard.press('g');expect(page,'s.paused && s.audience>0')
                page.keyboard.press(str(nation+1));expect(page,f'!s.paused && s.court_stages[{nation}]===1')
                assert state(page)['court_pledges'][nation]==nation+1
                page.keyboard.press('F6');page.keyboard.press('F6');expect(page,'s.journal_page===6')
                page.keyboard.press('u');expect(page,'s.political_goal===0')
                page.keyboard.press('u');expect(page,f's.political_goal==={nation+1}')
                page.keyboard.press('Enter');page.keyboard.press('m');expect(page,'s.map')
                page.screenshot(path=str(ARTIFACTS/f'court-{nation}-map.png'));page.keyboard.press('m')
            assert len(hashes)==3
            print('PASS three physical governments, audiences, deliberate decisions, route selection and paused clocks',flush=True)
            for outcome in range(1,4):
                page.goto(base+'?seed=713');wait_ready(page);start_play(page)
                import_fixture(page,f'courts-finale-{outcome}.sav')
                nation=outcome-1;s=state(page)
                assert s['generator']==13 and s['court']==3 and s['court_stages'][nation]==2,s
                if outcome==1:
                    page.keyboard.press('F2');expect(page,'s.players===2')
                page.keyboard.press('g');expect(page,'s.frontier_active')
                cursor=[640,360];page.mouse.move(*cursor)
                for _ in range(220):
                    s=state(page)
                    if s['local_state']&2:break
                    enemies=[e for e in s['regional_enemies'] if e['alive']]
                    if not enemies:
                        # Projectiles resolve after encounters in the simulation tick.
                        # The final death can precede FRONTIER_CLEARED by one tick.
                        print(f'Waiting for final-guard settlement, outcome {outcome}',flush=True)
                        expect(page,'(s.local_state&2)!==0')
                        break
                    aim_at(page,enemies[0]['at'],cursor)
                    page.mouse.down();expect(page,f"s.time>{s['time']+.15}");page.mouse.up()
                expect(page,'(s.local_state&2)!==0')
                page.keyboard.press('g');expect(page,f's.court_stages[{nation}]===3')
                earned=state(page);assert earned['court_claims'][nation]==(3 if outcome==1 else 1),earned
                page.keyboard.press('g');assert state(page)['credits']==earned['credits']
                page.keyboard.press('F6');page.keyboard.press('F6');expect(page,'s.journal_page===6')
                for _ in range(nation): page.keyboard.press('l')
                page.screenshot(path=str(ARTIFACTS/f'court-settlement-{outcome}.png'))
                page.keyboard.press('Enter');saved_at=state(page)['time'];page.keyboard.press('F5')
                expect(page,f's.time>{saved_at+1}');wait_for(page,'window.scStorage.status === 3')
                # A seed query deliberately starts a fresh expedition; resume without it.
                page.goto(base);wait_ready(page)
                resumed=state(page);assert resumed['court_stages'][nation]==3 and resumed['court_claims'][nation]==earned['court_claims'][nation],resumed
                assert resumed['players']==1 and resumed['director_sequence']>=earned['director_sequence']
            print('PASS three actual royal-guard victories, shared co-op settlement, exactly-once claims and IndexedDB resume',flush=True)
            context.close()
            mobile=browser.new_context(viewport={'width':390,'height':844},has_touch=True,is_mobile=True)
            touch=mobile.new_page();touch.goto(base+'?seed=713&scene=court1');wait_ready(touch);start_play(touch)
            # Actual touchscreen USE button rather than a writable simulation hook.
            client=mobile.new_cdp_session(touch)
            client.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':351,'y':228,'id':1}]})
            client.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
            expect(touch,'s.journal_page===6 && s.audience===2')
            touch.screenshot(path=str(ARTIFACTS/'court-mobile-audience.png'))
            touch.locator('#menu').tap();touch.locator('#story-choices button').nth(1).tap()
            expect(touch,'s.court_stages[1]===1 && !s.paused')
            touch.set_viewport_size({'width':844,'height':390})
            touch.locator('#menu').tap();touch.locator('[data-action="230"]').tap()
            expect(touch,'s.journal_page===6')
            touch.screenshot(path=str(ARTIFACTS/'court-landscape-journal.png'))
            assert not errors,errors
            browser.close()
            print('PASS touch audience/choice, portrait and landscape journals; no browser exceptions',flush=True)
    finally:
        server.shutdown();server.server_close()

if __name__=='__main__':run()
