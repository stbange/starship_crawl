"""Reactive systems, faction choices, debriefs and continuing stories in real WebGL."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, QuietHandler, start_play, state, wait_ready
from test_interiors_browser import expect, face_forward, walk_axis
from test_story_browser import import_fixture, save


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
        with sync_playwright() as p:
            options = {'headless': True, 'args': ['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader']}
            if os.environ.get('SC_BROWSER'):
                options['executable_path'] = os.environ['SC_BROWSER']
            else:
                options['channel'] = 'chromium'
            browser = p.chromium.launch(**options)
            context = browser.new_context(viewport={'width': 1280, 'height': 720})
            page = context.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=1&scene=bay')
            wait_ready(page)
            assert state(page)['story_version'] == 3
            start_play(page)

            import_fixture(page, 'saga-power.sav')
            expect(page, 's.story_tracked === 21 && !(s.systems & 1)')
            page.keyboard.press('g')
            expect(page, '(s.story_done & (1<<21)) !== 0 && (s.systems & 1) !== 0')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-powered-room.png'))
            points = state(page)['points']
            page.keyboard.press('F7')
            expect(page, 's.journal_page === 3 && s.paused')
            page.keyboard.press('1')
            assert state(page)['points'] == points  # Hidden journal keys cannot spend upgrades.
            page.keyboard.press('l')
            expect(page, 's.event_reading === 1')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-events.png'))
            page.keyboard.press('Enter')
            print('PASS powered room and event journal', flush=True)

            import_fixture(page, 'saga-beacon.sav')
            page.keyboard.press('g')
            expect(page, 's.safe && (s.systems & 3) === 3')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-beacon-ward.png'))
            page.keyboard.press('m')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-ward-map.png'))
            page.keyboard.press('m')
            save(page)
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            expect(page, 's.safe && (s.systems & 3) === 3')
            assert state(page)['story_done'] == saved['story_done']
            start_play(page)
            face_forward(page)
            before = state(page)
            page.keyboard.press('v')
            expect(page, 's.veil')
            walk_axis(page, 'z', before['z'] + 21, 'w')
            expect(page, '!s.safe')
            walk_axis(page, 'z', before['z'], 's')
            expect(page, 's.safe')
            print('PASS ward boundary, map, streaming state and IndexedDB', flush=True)

            import_fixture(page, 'saga-controller.sav')
            page.keyboard.press('g')
            expect(page, '(s.systems & 4) !== 0 && (s.story_done & (1<<25)) !== 0')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-controller-offline.png'))

            import_fixture(page, 'saga-council.sav')
            page.keyboard.press('g')
            expect(page, 's.decision && s.prepared && s.council === 0')
            assert 'BROKER' in state(page)['choice3']
            page.screenshot(path=str(ARTIFACTS / 'aftermath-council.png'))
            page.keyboard.press('3')
            expect(page, 's.council === 3 && !s.decision && s.prepared')
            snapshot = state(page)
            save(page)
            page.goto(base)
            wait_ready(page)
            for name in ('council', 'support0', 'support1', 'events', 'story_done', 'goal_x', 'goal_y', 'goal_z'):
                assert state(page)[name] == snapshot[name], (name, snapshot, state(page))
            start_play(page)

            import_fixture(page, 'saga-main.sav')
            assert state(page)['story_done'] & 3584 == 0  # Council assistance substitutes for the old optional operation.
            page.keyboard.press('g')
            expect(page, 's.decision && s.prepared')
            page.keyboard.press('3')
            expect(page, 's.story_choice === 3 && s.remaining === 0 && s.career4 === 0')
            page.keyboard.press('F7')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-pending-debrief.png'))
            page.keyboard.press('Enter')
            print('PASS factions, agreement persistence and assisted conclusion', flush=True)

            import_fixture(page, 'saga-continuation.sav')
            expect(page, 's.mission === 6 && s.career4 === 1 && s.story_tracked === 26')
            page.keyboard.press('F7')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-extraction-debrief.png'))
            page.keyboard.press('F4')
            expect(page, 's.journal_page === 0')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-character-career.png'))
            page.keyboard.press('Enter')
            snapshot = state(page)
            page.keyboard.press('F7')
            expect(page, 's.journal_page === 3 && s.paused')
            page.keyboard.press('g')
            expect(page, 's.chapter === 1 && s.story_tracked === 27 && s.career4 === 1 && !s.paused')
            assert state(page)['seed'] == snapshot['seed'] and state(page)['credits'] == snapshot['credits']
            page.keyboard.press('g')  # Repeated bay use cannot duplicate a chapter/reward.
            assert state(page)['chapter'] == 1 and state(page)['career4'] == 1
            page.keyboard.press('m')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-follow-on-map.png'))
            page.keyboard.press('m')

            import_fixture(page, 'saga-follow.sav')
            expect(page, 's.chapter === 1 && s.prepared && s.career4 === 1')
            page.keyboard.press('g')
            expect(page, 's.decision && s.journal_page === 1')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-follow-on-decision.png'))
            page.keyboard.press('3')
            expect(page, 's.chapter_choice === 3 && s.story_tracked === 30 && s.career4 === 1')
            save(page)
            page.goto(base)
            wait_ready(page)
            expect(page, 's.chapter_choice === 3 && s.story_tracked === 30 && s.career4 === 1')
            start_play(page)

            next_jobs = []
            for _ in range(2):
                import_fixture(page, 'saga-extracted.sav')
                expect(page, 's.chapter === 1 && s.total4 === 2 && s.career4 === 2')
                page.keyboard.press('g')
                expect(page, 's.chapter === 2 && s.story_tracked === 27 && s.career4 === 2')
                s = state(page)
                next_jobs.append(tuple(s[k] for k in ('seed', 'arc_kind', 'goal_x', 'goal_y', 'goal_z', 'support0', 'support1')))
            assert next_jobs[0] == next_jobs[1], next_jobs
            print('PASS extraction careers, two follow-ons and deterministic destinations', flush=True)
            context.close()

            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, has_touch=True, is_mobile=True, device_scale_factor=2)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=1&scene=bay&touch=1')
            wait_ready(page)
            start_play(page)
            import_fixture(page, 'saga-council.sav')
            page.locator('#menu').tap()
            page.locator('[data-action="8"]').tap()
            expect(page, 's.decision')
            page.locator('#menu').tap()
            page.locator('#menu').tap()
            assert page.locator('[data-action="92"]').is_enabled()
            page.locator('[data-action="92"]').tap()
            expect(page, 's.council === 3 && !s.paused')
            page.locator('#menu').tap()
            page.locator('[data-action="86"]').tap()
            expect(page, 's.journal_page === 3 && s.paused')
            page.screenshot(path=str(ARTIFACTS / 'aftermath-mobile-debrief.png'))
            page.locator('#menu').tap()
            page.locator('[data-action="88"]').tap()
            expect(page, 's.event_reading === 1')
            page.locator('#menu').tap()
            import_fixture(page, 'saga-continuation.sav')
            page.locator('#menu').tap()
            page.locator('[data-action="8"]').tap()
            expect(page, 's.chapter === 1 && s.story_tracked === 27')
            page.locator('#resume').tap()
            expect(page, '!s.paused && s.career4 === 1')
            save(page)
            page.goto(base)
            wait_ready(page)
            expect(page, 's.chapter === 1 && s.story_tracked === 27 && s.career4 === 1')
            assert not errors, errors
            browser.close()
        print('PASS aftermath browser: physical systems, faction agreement, debrief/events, persistent careers, continuing missions, deterministic sites and touch controls')
    except Exception:
        print('::error title=Aftermath browser regression::' + traceback.format_exc().replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A'), flush=True)
        raise
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    run()
