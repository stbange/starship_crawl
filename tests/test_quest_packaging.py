"""Packaging must fail before replacing an ordinary site or copying bad builds."""
from pathlib import Path
import subprocess
import re
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
            js=source/'starship_crawl_quest.js'
            js.write_text('immersive-vr /quest-persist starship_crawl_quest.wasm')
            wasm = source/'starship_crawl_quest.wasm'
            wasm.write_bytes(b'wrong')
            self.assertIn('WebAssembly', package(output/'quest').stderr)
            wasm.write_bytes(b'\0asm\x01\0\0\0')
            self.assertIn('separate directory', package(output).stderr)
            self.assertEqual(ordinary.read_bytes(), b'ordinary-build-retained')
            result = package(output/'quest')
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(ordinary.read_bytes(), b'ordinary-build-retained')
            def revision():
                html=(output/'quest/index.html').read_text()
                version=re.search(r'starship_crawl_quest\.([a-f0-9]{16})\.js',html).group(1)
                loader=(output/f'quest/starship_crawl_quest.{version}.js').read_text()
                self.assertIn(f'starship_crawl_quest.{version}.wasm',loader)
                self.assertEqual((output/f'quest/starship_crawl_quest.{version}.wasm').read_bytes(),wasm.read_bytes())
                return version
            first=revision()
            self.assertEqual(package(output/'quest').returncode,0)
            self.assertEqual(revision(),first)
            wasm.write_bytes(wasm.read_bytes()+b'changed-build')
            self.assertEqual(package(output/'quest').returncode,0)
            second=revision()
            self.assertNotEqual(second,first)
            js.write_text(js.read_text()+' changed-loader')
            self.assertEqual(package(output/'quest').returncode,0)
            self.assertNotEqual(revision(),second)
            self.assertIn('starship_crawl_quest.wasm',js.read_text())
            (source/'starship_crawl_quest.js').write_text('ordinary-game')
            self.assertIn('isolated Quest', package(output/'quest').stderr)


if __name__ == '__main__':
    unittest.main()
