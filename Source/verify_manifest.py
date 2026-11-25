from pathlib import Path
import json
import re
import sys

if len(sys.argv) != 3:
    raise SystemExit('usage: verify_manifest.py <manifest> <runtime-directory>')

manifest_path = Path(sys.argv[1])
runtime_dir = Path(sys.argv[2])
manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
identifier = re.compile(r'^[a-z0-9._-]{1,128}$')


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def valid_text(value, maximum, required=False):
    if not isinstance(value, str):
        return False
    if not value:
        return not required
    encoded = value.encode('utf-8')
    return len(encoded) <= maximum and all(ord(ch) >= 0x20 and ord(ch) != 0x7F for ch in value)


require(manifest.get('schemaVersion') == 1, 'schemaVersion must be 1')
require(identifier.fullmatch(manifest.get('id', '')) is not None, 'invalid plugin id')
require(manifest['id'] == 'ultimate-universe.enhanced-storage-management', 'unexpected plugin id')
require(valid_text(manifest.get('name', ''), 127, True), 'invalid name')
require(manifest['name'] == 'Enhanced Storage Management', 'unexpected display name')
require(valid_text(manifest.get('version', ''), 31, True), 'invalid version')
require(manifest['version'] == '1.0.1', 'unexpected version')
require(valid_text(manifest.get('author', ''), 127), 'invalid author')
require(valid_text(manifest.get('description', ''), 2048), 'invalid description')
require(manifest.get('dll') == 'EnhancedStorageManagement.dll', 'unexpected DLL name')
require(Path(manifest['dll']).name == manifest['dll'], 'DLL must be a basename')
require(manifest.get('loaderApi') == {'min': 1, 'max': 1}, 'loader API must be exactly 1')
require(manifest.get('supportedGameVersions') == ['1.2.0.8'], 'unexpected game-version support')

for field, maximum in [
    ('requiredSymbols', 32), ('optionalSymbols', 32),
    ('dependencies', 128), ('conflicts', 128), ('loadBefore', 128), ('loadAfter', 128),
]:
    values = manifest.get(field)
    require(isinstance(values, list) and len(values) <= maximum, f'invalid {field}')
    require(len(values) == len(set(values)), f'duplicate {field}')
    for value in values:
        require(identifier.fullmatch(value) is not None, f'invalid identifier in {field}: {value}')
        require(value != manifest['id'], f'{field} references plugin itself')

intents = manifest.get('hookIntents', [])
require(isinstance(intents, list) and len(intents) <= 64, 'invalid hookIntents')
for intent in intents:
    require(identifier.fullmatch(intent.get('symbol', '')) is not None, f"invalid hook symbol: {intent.get('symbol')}")
    require(valid_text(intent.get('interaction', ''), 63, True), 'invalid hook interaction')
    require(valid_text(intent.get('phase', ''), 15), 'invalid hook phase')
    require(valid_text(intent.get('temperature', ''), 15), 'invalid hook temperature')

require((runtime_dir / manifest['dll']).is_file(), 'manifest DLL is missing from runtime directory')
require((runtime_dir / 'EnhancedStorageManagement.ini').is_file(), 'runtime INI is missing')
require((runtime_dir / 'EnhancedStorageManagement.rml.json').resolve() == manifest_path.resolve(), 'manifest path/name mismatch')
require(not manifest['id'].startswith('tesmio.'), 'release identity must not use tesmio.* namespace')
print('PASS RML manifest verification')
print('id', manifest['id'])
print('name', manifest['name'])
print('dll', manifest['dll'])
