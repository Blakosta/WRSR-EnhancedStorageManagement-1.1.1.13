from pathlib import Path
import struct
import sys

if len(sys.argv) != 3:
    raise SystemExit('usage: finalize_pe.py <linked-dll> <output-dll>')

SOURCE = Path(sys.argv[1])
OUTPUT = Path(sys.argv[2])

PRODUCT_NAME = 'Enhanced Storage Management'
INTERNAL_NAME = 'EnhancedStorageManagement'
DLL_NAME = 'EnhancedStorageManagement.dll'
VERSION = '1.0.1'


def align(value, alignment):
    return (value + alignment - 1) & ~(alignment - 1)


def utf16z(text):
    return text.encode('utf-16le') + b'\0\0'


def version_block(key, value=b'', value_length=0, value_type=0, children=()):
    data = bytearray(b'\0' * 6)
    data += utf16z(key)
    while len(data) & 3:
        data += b'\0'
    data += value
    while len(data) & 3:
        data += b'\0'
    for child in children:
        data += child
        while len(data) & 3:
            data += b'\0'
    struct.pack_into('<HHH', data, 0, len(data), value_length, value_type)
    return bytes(data)


def version_string(key, value):
    encoded = utf16z(value)
    return version_block(key, encoded, len(value) + 1, 1)


# VS_FIXEDFILEINFO: 1.0.1.0 for both file and product versions.
fixed_info = struct.pack(
    '<13I',
    0xFEEF04BD,
    0x00010000,
    0x00010000,
    0x00000000,
    0x00010000,
    0x00000000,
    0x0000003F,
    0x00000000,
    0x00040004,
    0x00000002,
    0x00000000,
    0x00000000,
    0x00000000,
)

strings = [
    version_string('CompanyName', 'Ultimate-Universe'),
    version_string('FileDescription', PRODUCT_NAME),
    version_string('FileVersion', VERSION),
    version_string('InternalName', INTERNAL_NAME),
    version_string('LegalCopyright', 'Copyright (C) 2026 Ultimate-Universe'),
    version_string('OriginalFilename', DLL_NAME),
    version_string('ProductName', PRODUCT_NAME),
    version_string('ProductVersion', VERSION),
    version_string('Comments', 'Storage and freight logistics enhancement for Workers & Resources: Soviet Republic'),
]
string_table = version_block('040904B0', b'', 0, 1, strings)
string_info = version_block('StringFileInfo', b'', 0, 1, [string_table])
translation = version_block('Translation', struct.pack('<HH', 0x0409, 0x04B0), 4, 0)
var_info = version_block('VarFileInfo', b'', 0, 1, [translation])
version_data = version_block('VS_VERSION_INFO', fixed_info, len(fixed_info), 0, [string_info, var_info])

# RT_VERSION -> id 1 -> language 0x409 -> IMAGE_RESOURCE_DATA_ENTRY.
# This fixed 0x58-byte directory skeleton is generated from the PE resource
# structures directly; only the data RVA/size fields at 0x48/0x4C are patched.
resource = bytearray(bytes.fromhex(
    '00000000000000000000000000000100'
    '10000000180000800000000000000000'
    '00000000000001000100000030000080'
    '00000000000000000000000000000100'
    '09040000480000000000000000000000'
    'b004000000000000'
))
resource += version_data
resource_virtual_size = len(resource)


def pe_layout(data):
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise RuntimeError('not a PE image')
    section_count = struct.unpack_from('<H', data, pe + 6)[0]
    optional_size = struct.unpack_from('<H', data, pe + 20)[0]
    optional = pe + 24
    if struct.unpack_from('<H', data, optional)[0] != 0x20B:
        raise RuntimeError('expected PE32+')
    section_table = optional + optional_size
    sections = []
    for index in range(section_count):
        offset = section_table + index * 40
        name = data[offset:offset + 8].split(b'\0', 1)[0].decode('ascii', 'replace')
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from('<IIII', data, offset + 8)
        characteristics = struct.unpack_from('<I', data, offset + 36)[0]
        sections.append((name, virtual_size, virtual_address, raw_size, raw_offset, characteristics, offset))
    return pe, optional, section_table, section_count, sections


data = bytearray(SOURCE.read_bytes())
pe, optional, section_table, section_count, sections = pe_layout(data)
if any(section[0] == '.rsrc' for section in sections):
    raise RuntimeError('input already contains a resource section')

file_alignment = struct.unpack_from('<I', data, optional + 36)[0]
section_alignment = struct.unpack_from('<I', data, optional + 32)[0]
header_size = struct.unpack_from('<I', data, optional + 60)[0]
new_section_header = section_table + section_count * 40
if new_section_header + 40 > header_size:
    raise RuntimeError('no room for .rsrc section header')

last = max(sections, key=lambda section: section[2])
new_virtual_address = align(last[2] + max(last[1], last[3]), section_alignment)
new_raw_offset = align(len(data), file_alignment)
if len(data) < new_raw_offset:
    data += b'\0' * (new_raw_offset - len(data))

# The template has one IMAGE_RESOURCE_DATA_ENTRY at 0x48.
struct.pack_into('<I', resource, 0x48, new_virtual_address + 0x58)
struct.pack_into('<I', resource, 0x4C, len(version_data))
raw_size = align(len(resource), file_alignment)
data += resource
if len(resource) < raw_size:
    data += b'\0' * (raw_size - len(resource))

# IMAGE_SECTION_HEADER for .rsrc.
data[new_section_header:new_section_header + 8] = b'.rsrc\0\0\0'
struct.pack_into(
    '<IIIIIIHHI', data, new_section_header + 8,
    resource_virtual_size,
    new_virtual_address,
    raw_size,
    new_raw_offset,
    0, 0, 0, 0,
    0x40000040,  # initialized data, readable
)
struct.pack_into('<H', data, pe + 6, section_count + 1)

# DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].
struct.pack_into('<II', data, optional + 112 + 2 * 8, new_virtual_address, resource_virtual_size)
# SizeOfInitializedData and SizeOfImage.
struct.pack_into('<I', data, optional + 8, struct.unpack_from('<I', data, optional + 8)[0] + raw_size)
struct.pack_into('<I', data, optional + 56, align(new_virtual_address + resource_virtual_size, section_alignment))

# PE checksum.
checksum_offset = optional + 64
struct.pack_into('<I', data, checksum_offset, 0)
padded = data + (b'\0' if len(data) & 1 else b'')
checksum_total = 0
for offset in range(0, len(padded), 2):
    checksum_total += padded[offset] | (padded[offset + 1] << 8)
    checksum_total = (checksum_total & 0xFFFF) + (checksum_total >> 16)
checksum_total = (checksum_total & 0xFFFF) + (checksum_total >> 16)
checksum_total = (checksum_total & 0xFFFF) + (checksum_total >> 16)
checksum = (checksum_total + len(data)) & 0xFFFFFFFF
struct.pack_into('<I', data, checksum_offset, checksum)

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
OUTPUT.write_bytes(data)
print(f'{OUTPUT}: .rsrc RVA=0x{new_virtual_address:X}, size=0x{resource_virtual_size:X}, checksum=0x{checksum:X}')
