"""Load every retained checkpoint from completed runs using the ordinary loader.

Run analyze.py first to build the read-only inspection tool and summary. Failed
attempts remain in the summary; they are deliberately not counted as successes.
"""
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
rows = json.loads((HERE / 'summary.json').read_text())
results = []
for row in rows:
    if row['execution'].get('returncode') != 0:
        continue
    for file in sorted((HERE / row['run']).glob('*.sav')):
        result = subprocess.run([str(ROOT / 'build-tuning/sc_review_inspect'), str(file)],
                                capture_output=True, text=True)
        results.append({'path': str(file.relative_to(HERE)), 'returncode': result.returncode,
                        'error': result.stderr if result.returncode else None})
(HERE / 'checkpoint-validation.json').write_text(json.dumps(results, indent=2) + '\n')
print(f'Loaded {len(results)} checkpoints; {sum(bool(r["returncode"]) for r in results)} rejected')
raise SystemExit(any(r['returncode'] for r in results))
