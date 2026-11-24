from pathlib import Path
import hashlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('usage: verify_pe.py <dll>')

path = Path(sys.argv[1])
data = bytearray(path.read_bytes())


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def u16(offset):
    return struct.unpack_from('<H', data, offset)[0]


def u32(offset):
    return struct.unpack_from('<I', data, offset)[0]


require(data[:2] == b'MZ', 'missing DOS header')
pe = u32(0x3C)
require(data[pe:pe + 4] == b'PE\0\0', 'missing PE signature')
machine = u16(pe + 4)
section_count = u16(pe + 6)
optional_size = u16(pe + 20)
characteristics = u16(pe + 22)
require(machine == 0x8664, 'not AMD64')
require(characteristics & 0x2000, 'not marked as DLL')

optional = pe + 24
require(u16(optional) == 0x20B, 'not PE32+')
checksum_offset = optional + 64
checksum = u32(checksum_offset)
require(checksum != 0, 'PE checksum is zero')

check_data = data[:]
struct.pack_into('<I', check_data, checksum_offset, 0)
padded = check_data + (b'\0' if len(check_data) & 1 else b'')
total = 0
for offset in range(0, len(padded), 2):
    total += padded[offset] | (padded[offset + 1] << 8)
    total = (total & 0xFFFF) + (total >> 16)
total = (total & 0xFFFF) + (total >> 16)
total = (total & 0xFFFF) + (total >> 16)
calculated_checksum = (total + len(check_data)) & 0xFFFFFFFF
require(calculated_checksum == checksum, f'checksum mismatch {calculated_checksum:#x} != {checksum:#x}')

dll_characteristics = u16(optional + 70)
require(dll_characteristics & 0x20, 'HIGH_ENTROPY_VA missing')
require(dll_characteristics & 0x40, 'DYNAMIC_BASE missing')
require(dll_characteristics & 0x100, 'NX_COMPAT missing')

directory_count = u32(optional + 108)
require(directory_count >= 15, 'PE data directory table is incomplete')
directories = [struct.unpack_from('<II', data, optional + 112 + index * 8) for index in range(16)]
for index, name in [(0, 'export'), (1, 'import'), (2, 'resource'), (3, 'exception'), (5, 'base relocation')]:
    require(directories[index][0] and directories[index][1], f'{name} directory missing')
require(directories[6] == (0, 0), 'debug directory must be absent')
require(directories[14] == (0, 0), 'CLR directory must be absent')

section_table = optional + optional_size
sections = []
for index in range(section_count):
    offset = section_table + index * 40
    name = data[offset:offset + 8].split(b'\0', 1)[0].decode('ascii', 'replace')
    virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from('<IIII', data, offset + 8)
    flags = u32(offset + 36)
    writable = bool(flags & 0x80000000)
    executable = bool(flags & 0x20000000)
    require(not (writable and executable), f'RWX section: {name}')
    sections.append((name, virtual_size, virtual_address, raw_size, raw_offset, flags))

section_names = {section[0] for section in sections}
for required in ['.text', '.rdata', '.data', '.pdata', '.reloc', '.rsrc']:
    require(required in section_names, f'missing {required} section')


def rva_to_offset(rva):
    for _, virtual_size, virtual_address, raw_size, raw_offset, _ in sections:
        span = max(virtual_size, raw_size)
        if virtual_address <= rva < virtual_address + span:
            return raw_offset + (rva - virtual_address)
    raise AssertionError(f'RVA {rva:#x} not mapped by a section')


def c_string(offset):
    end = data.find(b'\0', offset)
    require(end >= 0, 'unterminated PE string')
    return bytes(data[offset:end]).decode('ascii', 'strict')

# Export names and export DLL identity.
export_rva = directories[0][0]
export_offset = rva_to_offset(export_rva)
export_name_rva = u32(export_offset + 12)
export_dll_name = c_string(rva_to_offset(export_name_rva))
require(export_dll_name == 'EnhancedStorageManagement.dll', f'unexpected export DLL name: {export_dll_name}')
export_name_count = u32(export_offset + 24)
export_names_rva = u32(export_offset + 32)
export_names_offset = rva_to_offset(export_names_rva)
export_names = set()
for index in range(export_name_count):
    name_rva = u32(export_names_offset + index * 4)
    export_names.add(c_string(rva_to_offset(name_rva)))
for required in ['TsmPluginApiVersion', 'TsmPluginInit', 'TsmPluginStart']:
    require(required in export_names, f'missing export {required}')

# Import table must remain conventional and minimal.
import_rva = directories[1][0]
import_offset = rva_to_offset(import_rva)
imports = {}
descriptor = import_offset
while True:
    original_first_thunk, timestamp, forwarder, name_rva, first_thunk = struct.unpack_from('<IIIII', data, descriptor)
    if not any((original_first_thunk, timestamp, forwarder, name_rva, first_thunk)):
        break
    dll_name = c_string(rva_to_offset(name_rva))
    thunk_rva = original_first_thunk or first_thunk
    thunk_offset = rva_to_offset(thunk_rva)
    names = []
    index = 0
    while True:
        entry = struct.unpack_from('<Q', data, thunk_offset + index * 8)[0]
        if entry == 0:
            break
        require((entry >> 63) == 0, 'ordinal import not expected')
        hint_name_offset = rva_to_offset(entry & 0x7FFFFFFFFFFFFFFF)
        names.append(c_string(hint_name_offset + 2))
        index += 1
    imports[dll_name.lower()] = names
    descriptor += 20
require('kernel32.dll' in imports, 'KERNEL32.dll import missing')
require('DisableThreadLibraryCalls' in imports['kernel32.dll'], 'DisableThreadLibraryCalls import missing')

# Exact release metadata and absence of development labels.
for text in [
    'Enhanced Storage Management',
    'EnhancedStorageManagement',
    'EnhancedStorageManagement.dll',
    '1.0.1',
    'Ultimate-Universe',
]:
    require(text.encode('utf-16le') in data, f'missing version resource string: {text}')
for forbidden in ['Test', 'prototype', 'development build']:
    require(forbidden.encode('utf-16le') not in data, f'development label in release metadata: {forbidden}')
require(b'.pdb' not in data.lower(), 'embedded PDB path found')

signature = data.find(struct.pack('<I', 0xFEEF04BD))
require(signature >= 0, 'VS_FIXEDFILEINFO missing')
require(u32(signature + 8) == 0x00010000, 'file version MS is not 1.0')
require(u32(signature + 12) == 0x00000000, 'file version LS is not 0.0')
require(u32(signature + 16) == 0x00010000, 'product version MS is not 1.0')
require(u32(signature + 20) == 0x00000000, 'product version LS is not 0.0')

print('PASS PE verification')
print('sha256', hashlib.sha256(data).hexdigest())
print('checksum', hex(checksum))
print('export_dll', export_dll_name)
print('exports', sorted(export_names))
print('imports', imports)
print('sections', [(name, hex(flags)) for name, _, _, _, _, flags in sections])
print('size', len(data))
