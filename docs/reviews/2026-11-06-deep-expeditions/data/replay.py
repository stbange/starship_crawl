import sys,threading,json,time,subprocess,os
from pathlib import Path
from functools import partial
from http.server import ThreadingHTTPServer
sys.path.insert(0,str(Path(__file__).resolve().parents[4]/'tests'))
from playwright.sync_api import sync_playwright
from test_browser import QuietHandler,wait_ready,start_play,state
from test_story_browser import import_fixture
root=Path(__file__).resolve().parents[1]
server=ThreadingHTTPServer(('127.0.0.1',0),partial(QuietHandler,directory=os.environ.get('SC_REVIEW_SITE',str(root.parents[2]/'build-pages-preview'))))
threading.Thread(target=server.serve_forever,daemon=True).start()
names=['inward','east','west','up','down','east-inward','west-inward','up-inward','down-inward','east-down-inward']
with sync_playwright() as p:
 b=p.chromium.launch(headless=True,executable_path=os.environ.get('SC_BROWSER','/usr/bin/chromium'),args=['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader'])
 for i,name in enumerate(names):
  d=root/'data'/f'{i:02}-{name}'
  while not (d/'execution.json').exists():time.sleep(2)
  ctx=b.new_context(viewport={'width':1280,'height':720});page=ctx.new_page();page.on('dialog',lambda q:q.accept())
  page.goto(f'http://127.0.0.1:{server.server_port}/starship_crawl/?seed=713');wait_ready(page);start_play(page)
  for tag in ['minute-15','minute-45','deepest']:
   source=d/f'phase-{tag}.sav'
   if not source.exists():print('MISSING',source,flush=True);continue
   check=subprocess.run(['/tmp/check-deep',str(source)],capture_output=True,text=True)
   assert check.returncode==0,(source,check.stderr)
   import_fixture(page,str(source));s=state(page);assert s['seed']==713 and not s['failed'] and s['rank']>1,s
   target=root/'screenshots'/f'{i:02}-{name}-{tag}'
   target.with_suffix('.json').write_text(json.dumps({'render':s,'checkpoint_support_check':check.stdout.strip()},indent=2)+'\n')
   page.screenshot(path=str(target.with_suffix('.png')))
   print(i,name,tag,s['x'],s['y'],s['z'],check.stdout.strip(),flush=True)
  ctx.close()
 b.close()
server.shutdown()
