"""Exercise missing, old, current and broken library installations without network."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('deps', ROOT / 'scripts/install_dependencies.py')
deps = importlib.util.module_from_spec(spec)
spec.loader.exec_module(deps)


class DependenciesTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.inventory = {'installed_libraries': []}
        for entry in deps.LIBRARIES + [{'name': 'micro_ros_arduino', 'version': '2.0.8-jazzy'}]:
            directory = Path(self.temp.name) / entry['name']
            directory.mkdir()
            (directory / deps.HEADERS[entry['name']]).write_text('// fixture')
            self.inventory['installed_libraries'].append({'library': dict(entry, source_dir=str(directory))})

    def test_complete(self):
        self.assertTrue(all(row[0] for row in deps.dependency_status(self.inventory)))

    def test_missing(self):
        result = deps.dependency_status({})
        self.assertEqual(len(result), 5)
        self.assertTrue(all(not row[0] for row in result))

    def test_old_and_newer_versions(self):
        library = self.inventory['installed_libraries'][0]['library']
        library['version'] = '0.4.7'
        self.assertFalse(deps.dependency_status(self.inventory)[0][0])
        library['version'] = '0.5.0'
        self.assertTrue(deps.dependency_status(self.inventory)[0][0])

    def test_header_missing(self):
        library = self.inventory['installed_libraries'][0]['library']
        (Path(library['source_dir']) / 'MPU9250.h').unlink()
        self.assertFalse(deps.dependency_status(self.inventory)[0][0])

    def test_ros_optional_and_wrong_distribution(self):
        self.inventory['installed_libraries'][-1]['library']['version'] = '3.0.0-iron'
        self.assertFalse(deps.dependency_status(self.inventory)[-1][0])
        self.assertTrue(all(row[0] for row in deps.dependency_status(self.inventory, False)))


if __name__ == '__main__':
    unittest.main()
