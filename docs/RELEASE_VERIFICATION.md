# 1.0.1 Release Verification

Enhanced Storage Management 1.0.1 is a narrow placement-feedback and shared-hook compatibility patch over the working 1.0.0 storage mechanics baseline.

## Static/build verification

Passed:

- strict native C++ build with `/W4 /WX`;
- AMD64 PE32+ DLL;
- normal `KERNEL32.dll!DisableThreadLibraryCalls` import;
- ASLR, high-entropy VA and NX/DEP enabled;
- relocation and x64 unwind tables present;
- non-zero valid PE checksum;
- no RWX section, CLR payload, debug directory or embedded PDB path;
- embedded Windows version metadata reports Enhanced Storage Management 1.0.1;
- RML manifest identity is `ultimate-universe.enhanced-storage-management`;
- player runtime payload contains only DLL, INI and RML manifest;
- packaged source reproduces the exact release DLL byte-for-byte.

Release DLL SHA-256:

`2c1b12740d362c1ad3b4b59fd93e0754c18289035ceac48b1a58938cbfb0c183`

## Runtime verification

Passed in a full RML plugin stack on WRSR 1.1.1.9:

- Enhanced Storage Management 1.0.1 loaded and remained active;
- five intended ESM hook targets installed;
- `0x139A70` simulation dispatcher chained as ESM → Dock Distribution Office → original WRSR;
- `0x2F0F10` construction frame chained as ESM → Parallel Utilities → ForceNodes → original WRSR;
- ESM's resource-transfer, storage-UI and factory/storage-query hooks remained direct healthy owners;
- 270 read-only diagnostic monitor scans observed zero late executable-change windows;
- final hook-chain audit issues: 0;
- WRSR exited normally with code `0x0` after approximately 9 minutes 16 seconds;
- manual gameplay observation reported the mod working and the revised placement behavior functioning.

The remaining warnings in that session were attributed to other plugins and did not break an ESM chain. See `RUNTIME_VALIDATION_v1.0.1.md`.
