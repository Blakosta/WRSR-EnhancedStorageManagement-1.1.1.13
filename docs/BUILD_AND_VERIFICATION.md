# Build and Verification

## Toolchain

From the repository/source archive, enter `Source/` first.

The release build uses `clang-cl` and `lld-link` as a native Windows x64, no-CRT DLL build.

On Linux, WSL, or another shell environment with the Windows-targeting LLVM tools:

```bash
./build_release.sh
```

On Windows with `clang-cl`, `lld-link`, and the Python launcher in `PATH`:

```bat
build_release.bat
```

The shell script also accepts an explicit destination directory:

```bash
./build_release.sh /path/to/output
```

The script compiles with:

- `/O2`
- `/GR-`
- `/EHs-c-`
- `/Zl`
- `/W4 /WX`
- `/GS-` because this deliberately no-CRT build does not link the Microsoft security-cookie runtime

and links with:

- `/machine:x64`
- `/nodefaultlib`
- `/dynamicbase`
- `/highentropyva`
- `/nxcompat`
- `/timestamp:0`
- `/opt:ref /opt:icf`

`kernel32_min.def` is converted into a minimal conventional import library at build time and supplies `KERNEL32.dll!DisableThreadLibraryCalls`. The native whole-building highlight is invoked directly from the construction lifecycle using the Windows x64 vararg ABI; no assembly thunk is required.

## Deterministic output

`/timestamp:0`, the fixed VERSIONINFO generator and deterministic linker options are used so an unchanged source tree rebuilds byte-identically with the same toolchain.

## Automated checks

`build_release.sh` performs:

1. strict `/W4 /WX` compilation;
2. final Windows VERSIONINFO insertion;
3. exact RML manifest validation, including the neutral plugin identity;
4. PE structural validation;
5. SHA-256 reporting.

`verify_pe.py` checks:

- AMD64 PE32+ DLL;
- nonzero and correct PE checksum;
- `HIGH_ENTROPY_VA`, `DYNAMIC_BASE`, `NX_COMPAT`;
- export/import/resource/exception/relocation directories;
- no debug directory;
- no CLR header;
- no writable+executable section;
- `.text`, `.rdata`, `.data`, `.pdata`, `.reloc`, `.rsrc` presence;
- exact legacy compatibility exports;
- conventional `KERNEL32.dll!DisableThreadLibraryCalls` import;
- no embedded PDB path;
- final `Enhanced Storage Management` / `1.0.1` version strings;
- fixed numeric file/product version `1.0.1.0`;
- absence of stale pre-release product/version metadata.

The x64 unwind table can additionally be inspected with:

```bash
llvm-objdump --unwind-info EnhancedStorageManagement.dll
```

The final build must show valid x64 unwind records for compiler-generated non-leaf functions. Version 1.0.1 no longer uses the experimental assembly highlight thunk.
