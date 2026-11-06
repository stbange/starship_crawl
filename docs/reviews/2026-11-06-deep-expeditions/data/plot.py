from pathlib import Path
import csv,math
import matplotlib;matplotlib.use('Agg')
import matplotlib.pyplot as plt
root=Path(__file__).resolve().parents[1]
fig,axs=plt.subplots(1,2,figsize=(12,4),layout='constrained')
for d in sorted((root/'data').glob('[0-9][0-9]-*')):
 rows=[r for r in csv.reader((d/'run.log').read_text().splitlines()) if r and r[0]=='SAMPLE']
 times=[float(r[2])/60 for r in rows];layers=[]
 for r in rows:
  x,y,z=map(float,r[3:6]);cx=math.floor(x/96+.5);cy=math.floor((y+.001)/64);cz=max(0,math.floor(z/96))
  layers.append(1+int(max(((-cx-1)/4 if cx<0 else cx/4),abs(cy)/2,cz/4)))
 ax=axs[1] if d.name in ['03-up','04-down'] else axs[0]
 ax.plot(times,layers,label=d.name[3:])
for ax,title in zip(axs,['Routes combining walking and combat','Repeated lift travel (different vertical scale)']):
 ax.set(xlabel='Simulated minutes',ylabel='Coordinate layer',title=title);ax.grid(alpha=.25);ax.legend(fontsize=7)
fig.suptitle('Ten one-hour expeditions — seed 713, perfect-aim controller')
fig.savefig(root/'depth-over-time.png',dpi=160)
