"""Packaging must fail before replacing an ordinary site or copying bad builds."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class QuestPackaging(unittest.TestCase):
    def test_staging_and_rejection(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'build'
            source.mkdir()
            output = root / 'site'
            output.mkdir()
            ordinary = output / 'starship_crawl.js'
            ordinary.write_bytes(b'ordinary-build-retained')

            def package(destination):
                return subprocess.run([sys.executable, str(ROOT/'scripts/package-quest.py'),
                    '--build-dir', str(source), '--output', str(destination)], capture_output=True, text=True)

            self.assertNotEqual(package(output/'quest').returncode, 0)
            (source/'starship_crawl_quest.html').write_text('<script src="starship_crawl_quest.js"></script>')
            (source/'starship_crawl_quest.js').write_text('immersive-vr /quest-persist')
            wasm = source/'starship_crawl_quest.wasm'
            wasm.write_bytes(b'wrong')
            self.assertIn('WebAssembly', package(output/'quest').stderr)
            wasm.write_bytes(b'\0asm\x01\0\0\0')
            self.assertIn('separate directory', package(output).stderr)
            self.assertEqual(ordinary.read_bytes(), b'ordinary-build-retained')
            result = package(output/'quest')
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((output/'quest/starship_crawl_quest.wasm').read_bytes(), wasm.read_bytes())
            self.assertEqual(ordinary.read_bytes(), b'ordinary-build-retained')
            (source/'starship_crawl_quest.js').write_text('ordinary-game')
            self.assertIn('isolated Quest', package(output/'quest').stderr)


if __name__ == '__main__':
    unittest.main()
