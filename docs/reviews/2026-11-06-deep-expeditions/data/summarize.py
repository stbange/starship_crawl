from pathlib import Path
import json,csv,re,collections,math
root=Path('docs/reviews/2026-11-06-deep-expeditions');out=[]
for d in sorted((root/'data').glob('[0-9][0-9]-*')):
 if not (d/'execution.json').exists():continue
 lines=(d/'run.log').read_text().splitlines()
 samples=[r for r in csv.reader(lines) if r and r[0]=='SAMPLE']
 cells=[r for r in csv.reader(lines) if r and r[0]=='CELL']
 end=samples[-1];deep=re.search(r'DEPTH route=(\d+) maxlayer=(\d+) stage=(\d+) recoveries=(\d+)', '\n'.join(lines))
 phase4=next(float(x.rsplit(' at ',1)[1][:-1]) for x in lines if x.startswith('Seed ') and 'phase 4,' in x)
 late=[r for r in cells if float(r[1])>1800]
 count=lambda i:dict(collections.Counter(int(r[i]) for r in late))
 result={**json.loads((d/'execution.json').read_text()),'sim_seconds':float(end[2]),'phase4_seconds':phase4,'kills':int(end[6]),'deaths':int(end[7]),'rank':int(end[8]),'max_coordinate_layer':int(deep[2]),'stage':int(deep[3]),'recoveries':int(deep[4]),'end_xyz':[float(x) for x in end[3:6]],'unique_cells':len(set(tuple(r[2:5]) for r in cells)),'unique_themes':len(set(r[6] for r in cells)),'macro_forms':sorted(set(int(r[7]) for r in cells)),'late_cell_visits':len(late),'late_themes':count(6),'late_conditions':count(8),'late_heat':count(9),'late_order':count(10),'late_alien':count(11)}
 out.append(result)
(root/'data/summary.json').write_text(json.dumps(out,indent=2)+'\n')
for r in out:print(r['name'],r['sim_seconds'],r['max_coordinate_layer'],r['end_xyz'],r['kills'],r['deaths'],'recoveries',r['recoveries'],'late',r['late_conditions'])
