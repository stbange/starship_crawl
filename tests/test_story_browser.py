"""Generated-story discovery, journals and decisions through the packaged WebGL UI."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import threading
import traceback
from playwright.sync_api import sync_playwright
from test_browser import ARTIFACTS, BUILD, ROOT, QuietHandler, start_play, state, wait_for, wait_ready
from test_interiors_browser import expect, face_forward, walk_axis


def save(page):
    page.evaluate("Module.ccall('sc_web_save',null,[],[])")
    wait_for(page, 'window.scStorage.status === 3')


def import_fixture(page, name):
    if page.evaluate('!!document.pointerLockElement'):
        page.keyboard.press('Escape')
        expect(page, 's.paused')
    page.locator('#menu').click()
    page.locator('#import').set_input_files(str(ROOT / 'tests' / 'fixtures' / name))
    expect(page, '!s.paused && s.pending === 0')


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
            for motivation, seed in enumerate((1, 3, 4, 14)):
                page.goto(base + f'?seed={seed}&scene=bay')
                wait_ready(page)
                assert state(page)['story_version'] == 2 and state(page)['motivation'] == motivation
                assert state(page)['story_assigned'] == 1 and state(page)['story_done'] == 0
                start_play(page)
                page.keyboard.press('i')
                expect(page, 's.journal && s.journal_page === 1 && s.paused')
                page.screenshot(path=str(ARTIFACTS / f'story-briefing-{motivation}.png'))
                page.keyboard.press('F4')
                expect(page, 's.journal && s.journal_page === 0')
                page.keyboard.press('Enter')
                expect(page, '!s.journal && !s.paused')

            # Enter a real cause-matched room from the bay after accepting the job.
            # Seed 22 has a reachable command-origin room two cells to the east.
            page.goto(base + '?seed=22&scene=bay')
            wait_ready(page)
            start_play(page)
            page.keyboard.press('F6')
            expect(page, 's.journal && s.journal_page === 2 && s.paused && !(s.story_seen & 262144)')
            page.screenshot(path=str(ARTIFACTS / 'history-dossier-unknown.png'))
            page.keyboard.press('i')
            expect(page, 's.journal && s.journal_page === 1 && s.paused')
            page.keyboard.press('F6')
            expect(page, 's.journal_page === 2 && s.paused')
            page.keyboard.press('Enter')
            face_forward(page)
            walk_axis(page, 'z', 43, 'w')
            page.keyboard.press('g')
            expect(page, 's.mission === 1')
            page.keyboard.press('v')
            expect(page, 's.veil')
            walk_axis(page, 'z', 48, 'w')
            for x in (30, 60, 90, 120, 150, 170):
                walk_axis(page, 'x', x, 'd')
            expect(page, 's.history_role === 1 && (s.story_seen & 262144) !== 0 && s.mission === 1 && s.credits >= 75 && s.points === 2')
            page.screenshot(path=str(ARTIFACTS / 'history-command-origin.png'))
            page.keyboard.press('F6')
            expect(page, 's.journal_page === 2 && s.paused')
            page.screenshot(path=str(ARTIFACTS / 'history-dossier-discovered.png'))
            save(page)
            snapshot = state(page)
            page.goto(base)
            wait_ready(page)
            expect(page, '(s.story_seen & 262144) !== 0 && s.history_role === 1 && s.mission === 1')
            assert state(page)['story_seen'] == snapshot['story_seen']
            start_play(page)

            page.goto(base + '?seed=1&scene=bay')
            wait_ready(page)
            start_play(page)
            face_forward(page)
            walk_axis(page, 'z', 43, 'w')
            page.keyboard.press('g')
            expect(page, 's.mission === 1 && s.credits >= 75')
            # Walk through the real bay/engine doorway; streaming alone never finds this clue.
            page.keyboard.press('v')
            expect(page, 's.veil')
            walk_axis(page, 'z', 48, 'w')
            walk_axis(page, 'x', 30, 'd')
            walk_axis(page, 'x', 60, 'd')
            expect(page, '(s.story_seen & 62) !== 0 && (s.story_assigned & 128) !== 0')
            page.keyboard.press('u')
            expect(page, 's.story_tracked === 7')
            snapshot = state(page)
            page.keyboard.press('m')
            page.keyboard.press('ArrowRight')
            page.keyboard.press('PageUp')
            expect(page, 's.map')
            after = state(page)
            assert after['story_done'] == snapshot['story_done'] and after['story_assigned'] == snapshot['story_assigned']
            page.screenshot(path=str(ARTIFACTS / 'story-tracked-lead-map.png'))
            page.keyboard.press('m')
            page.keyboard.press('i')
            expect(page, 's.journal && s.paused')
            page.keyboard.press('l')
            expect(page, 's.story_reading > 0')
            page.screenshot(path=str(ARTIFACTS / 'story-discovery-journal.png'))
            save(page)
            saved = state(page)
            page.goto(base)
            wait_ready(page)
            loaded = state(page)
            for field in ('story_assigned', 'story_seen', 'story_done', 'story_tracked', 'goal_x', 'goal_y', 'goal_z', 'credits'):
                assert loaded[field] == saved[field], (field, loaded, saved)
            start_play(page)

            # These real format-7 saves were produced by the movement/combat campaign,
            # after optional preparation. Import them through the ordinary player UI.
            for motivation in range(3):
                import_fixture(page, f'story-{motivation}-choice3.sav')
                expect(page, f's.motivation === {motivation} && s.story_choice === 0')
                page.screenshot(path=str(ARTIFACTS / f'story-site-{motivation}.png'))
                if motivation == 0:
                    page.keyboard.press('F6')
                    expect(page, 's.journal_page === 2 && s.paused && s.story_version === 1')
                    page.screenshot(path=str(ARTIFACTS / 'history-archival-dossier.png'))
                    page.keyboard.press('Enter')
                page.keyboard.press('g')
                expect(page, 's.decision && s.journal && s.paused')
                page.screenshot(path=str(ARTIFACTS / f'story-decision-{motivation}.png'))
                if motivation == 0:
                    page.keyboard.press('Escape')
                    expect(page, '!s.decision && !s.journal && !s.paused && s.story_choice === 0')
                    page.keyboard.press('g')
                    expect(page, 's.decision && s.paused')
                page.keyboard.press('3')
                expect(page, 's.story_choice === 3 && !s.decision && !s.paused && s.story_tracked === 15')
                page.keyboard.press('i')
                page.screenshot(path=str(ARTIFACTS / f'story-consequence-{motivation}.png'))
                save(page)
                page.goto(base)
                wait_ready(page)
                expect(page, f's.motivation === {motivation} && s.story_choice === 3 && s.story_tracked === 15')
                start_play(page)

            # New direct commissions retain their independent targets; containment
            # points to the physical origin. Fixtures come from the real C journey.
            for motivation in range(4):
                import_fixture(page, f'history-{motivation}-arc0-choice3.sav')
                expect(page, f's.story_version === 2 && s.motivation === {motivation} && s.prepared && s.story_choice === 0')
                choices = state(page)
                if motivation == 0:
                    assert 'EVERYONE HERE GOES HOME' in choices['choice1']
                elif motivation == 1:
                    assert 'COMPLETE THE COMMISSION' in choices['choice1']
                elif motivation == 2:
                    assert 'SPARE DRIVE' in choices['choice1']
                else:
                    assert choices['history_role'] == 1 and choices['story_seen'] & 262144
                    assert tuple(choices['goal_' + a] for a in ('x', 'y', 'z')) == tuple(choices['origin_' + a] for a in ('x', 'y', 'z'))
                page.keyboard.press('g')
                expect(page, 's.decision && s.paused')
                page.screenshot(path=str(ARTIFACTS / f'history-decision-{motivation}.png'))
                page.keyboard.press('3')
                expect(page, 's.story_choice === 3 && s.story_tracked === 15 && !s.paused')
                save(page)
                page.goto(base)
                wait_ready(page)
                expect(page, 's.story_version === 2 && s.story_choice === 3 && s.story_tracked === 15')
                start_play(page)
            import_fixture(page, 'history-3-arc0-choice1.sav')
            expect(page, '!s.prepared && s.history_role === 1')
            page.keyboard.press('g')
            expect(page, 's.decision')
            page.keyboard.press('1')
            expect(page, 's.story_choice === 1 && !s.paused')
            save(page)
            page.goto(base)
            wait_ready(page)
            expect(page, 's.story_choice === 1 && s.history_role === 1')
            start_play(page)

            context.close()
            mobile = browser.new_context(viewport={'width': 844, 'height': 390}, has_touch=True, is_mobile=True, device_scale_factor=2)
            page = mobile.new_page()
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.on('dialog', lambda dialog: dialog.accept())
            page.goto(base + '?seed=1&scene=bay&touch=1')
            wait_ready(page)
            start_play(page)
            import_fixture(page, 'story-0-choice1.sav')
            page.locator('#menu').tap()
            page.locator('[data-action="8"]').tap()
            expect(page, 's.decision && s.paused')
            page.locator('#menu').tap()
            page.locator('#menu').tap()
            assert page.locator('[data-action="92"]').is_disabled()
            page.screenshot(path=str(ARTIFACTS / 'story-mobile-choices.png'))
            page.locator('[data-action="91"]').tap()
            expect(page, 's.story_choice === 2 && !s.decision && !s.paused')
            page.locator('#menu').tap()
            page.locator('[data-action="80"]').tap()
            expect(page, 's.journal && s.journal_page === 1')
            page.screenshot(path=str(ARTIFACTS / 'story-mobile-journal.png'))
            save(page)
            page.goto(base)
            wait_ready(page)
            expect(page, 's.story_choice === 2 && s.story_tracked === 15')
            start_play(page)
            import_fixture(page, 'history-3-arc0-choice3.sav')
            page.locator('#menu').tap()
            page.locator('[data-action="85"]').tap()
            expect(page, 's.journal_page === 2 && s.paused')
            page.screenshot(path=str(ARTIFACTS / 'history-mobile-dossier.png'))
            page.locator('#menu').tap()
            page.locator('#resume').tap()
            page.locator('#menu').tap()
            page.locator('[data-action="8"]').tap()
            expect(page, 's.decision && s.paused')
            page.locator('#menu').tap()
            page.locator('#menu').tap()
            assert page.locator('[data-action="92"]').is_enabled()
            page.screenshot(path=str(ARTIFACTS / 'history-mobile-isolation.png'))
            page.locator('[data-action="92"]').tap()
            expect(page, 's.story_choice === 3 && !s.paused && !s.decision')
            assert not errors, errors
            browser.close()
        print('PASS narrative browser: four briefings, physical origin discovery, dossier, map lead, old/new IndexedDB saves, direct and containment decisions, mobile locked/prepared choices')
    except Exception:
        detail = traceback.format_exc().replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
        print('::error title=Narrative browser regression::' + detail, flush=True)
        raise
    finally:
        server.shutdown()


if __name__ == '__main__':
    run()
