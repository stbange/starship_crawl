"""P1 real-input WebGL checks; inspection scenes never save or grant progress."""
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
from test_story_browser import import_fixture


def aim_at(page, at, cursor):
    s = state(page)
    dx, dy, dz = at[0]-s['x'], at[1]-(s['y']+1.6), at[2]-s['z']
    yaw = math.atan2(dx, dz)-s['yaw']
    yaw = math.atan2(math.sin(yaw), math.cos(yaw))
    pitch = math.atan2(dy, math.hypot(dx,dz))-s['pitch']
    cursor[0] += yaw/.0023
    cursor[1] -= pitch/.0023
    page.mouse.move(*cursor)
    expect(page, f"s.time > {s['time']+.04}")


def run():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-dir',type=Path,default=BUILD)
    parser.add_argument('--url-path',default='/starship_crawl.html')
    args=parser.parse_args()
    ARTIFACTS.mkdir(parents=True,exist_ok=True)
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(QuietHandler,directory=str(args.site_dir.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    base=f'http://127.0.0.1:{server.server_port}{args.url_path}'
    try:
        with sync_playwright() as pw:
            browser=pw.chromium.launch(headless=True,executable_path=os.environ.get('SC_BROWSER','/usr/bin/chromium'),
                args=['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader'])
            context=browser.new_context(viewport={'width':1280,'height':720},accept_downloads=True)
            page=context.new_page();errors=[]
            page.on('pageerror',lambda e:errors.append(str(e)))
            page.on('dialog',lambda d:d.accept())
            for scene in ['relay','nest']:
                page.goto(base+f'?seed=713&scene={scene}');wait_ready(page);start_play(page)
                s=state(page)
                assert s['generator']==11 and s['pace']==3 and s['frontier_living']==4 and not s['frontier_active'],s
                assert s['alien_behaviors']==30 and s['frontier_offer'],s
                page.screenshot(path=str(ARTIFACTS/f'regional-{scene}-dormant.png'))
                page.mouse.down();expect(page,f"s.time > {s['time']+.7}");page.mouse.up()
                assert state(page)['frontier_core']==s['frontier_core']
                page.keyboard.press('g');expect(page,'s.frontier_active && s.frontier_attacker > 0')
                expect(page,'(s.alien_motions & 8) !== 0')
                page.screenshot(path=str(ARTIFACTS/f'regional-{scene}-links.png'))
                page.keyboard.press('Escape');expect(page,'s.paused')
                frozen=state(page);expect(page,f"s.frames > {frozen['frames']+5}")
                assert state(page)['time']==frozen['time'] and state(page)['regional_enemies']==frozen['regional_enemies']
                page.keyboard.press('Enter');expect(page,'!s.paused')
                page.keyboard.press('F2');expect(page,'s.players === 2 && (s.local_state & 12) === 12')
                page.screenshot(path=str(ARTIFACTS/f'regional-{scene}-split.png'))
            page.goto(base+'?seed=713&scene=local4');wait_ready(page);start_play(page)
            page.screenshot(path=str(ARTIFACTS/'regional-local-before.png'))
            page.keyboard.press('g');expect(page,'(s.local_state & 1) !== 0')
            credits=state(page)['credits'];page.keyboard.press('g')
            assert state(page)['credits']==credits
            page.screenshot(path=str(ARTIFACTS/'regional-local-restored.png'))
            print('PASS optional activation, warnings, animation pause, shared co-op and local repair',flush=True)
            # Checkpoint provenance: normal gameplay journey, no scene or test grant.
            page.goto(base+'?seed=713');wait_ready(page);start_play(page)
            import_fixture(page,'regional-comparison.sav')
            expect(page,'s.pace === 3 && !s.frontier_active && s.weapon === 17')
            page.keyboard.press('g');expect(page,'s.frontier_active')
            cursor=[640,360];page.mouse.move(*cursor)
            disrupted=False
            for _ in range(120):
                s=state(page);disrupted |= s['frontier_disrupt']>0
                if s['local_state'] & 2:break
                alive=[e for e in s['regional_enemies'] if e['alive']]
                if not alive:
                    # The last projectile can kill after this tick's encounter check.
                    expect(page,'(s.local_state & 2) !== 0')
                    break
                # Shoot a shielded guard first, so the beam must pressure a real link.
                target=alive[-1]
                aim_at(page,target['at'],cursor)
                page.mouse.down();expect(page,f"s.time > {s['time']+.12}");page.mouse.up()
            expect(page,'(s.local_state & 2) !== 0')
            assert disrupted
            before_claim=state(page)
            page.keyboard.press('g');expect(page,'(s.local_state & 16) !== 0 && s.drill_time > 0')
            page.screenshot(path=str(ARTIFACTS/'regional-earned-reward.png'))
            earned=state(page)
            # This checkpoint earns VEIL after the already-owned Prism Lance.
            assert earned['powers_owned'] != before_claim['powers_owned'] and earned['power']==0
            page.keyboard.press('v')
            expect(page,'s.veil && s.drill_complete')
            before=state(page);page.keyboard.press('F5')
            expect(page,f"s.time > {before['time']+.12}")
            wait_for(page,'window.scStorage.status === 3')
            before=state(page);page.goto(base);wait_ready(page)
            assert state(page)['local_state'] & 16
            assert state(page)['kills']==before['kills'] and state(page)['frontier_living']==0
            page.screenshot(path=str(ARTIFACTS/'regional-saved-victory.png'))
            (ARTIFACTS/'regional-victory.json').write_text(json.dumps(state(page),indent=2))
            assert not errors,errors
            context.close()
            mobile=browser.new_context(viewport={'width':844,'height':390},is_mobile=True,has_touch=True)
            page=mobile.new_page();page.goto(base+'?seed=713&scene=relay');wait_ready(page);start_play(page)
            client=mobile.new_cdp_session(page)
            client.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':760,'y':105,'id':1}]})
            client.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
            expect(page,'s.frontier_active')
            page.screenshot(path=str(ARTIFACTS/'regional-mobile.png'))
            page.set_viewport_size({'width':390,'height':844})
            page.screenshot(path=str(ARTIFACTS/'regional-mobile-portrait.png'))
            assert not state(page)['failed']
            mobile.close();browser.close()
            print('PASS earned beam disruption, claims, field practice, save/reload and touch USE')
    finally:
        server.shutdown()

if __name__=='__main__':
    run()
