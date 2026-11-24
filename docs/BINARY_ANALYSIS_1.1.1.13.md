# Binary analysis: WRSR 1.1.1.13

This note records observations made against the exact game binaries supplied for the 1.1.1.13 adaptation. It is not a release certification.

## Input binaries

- `SOVIET64.exe` SHA-256: `4df5fbecc75300e3b843ae4e6426761c84ae2e989321c011c6f560be79122125`
- `C3DDLL64.dll` SHA-256: `5d8a0540a01170475392b4650f492582fab4ed20eec8755190a053f410247e3a`

## Verified executable function entries

The following entry signatures from the upstream 1.1.1.9 source were searched in the supplied 1.1.1.13 executable. The listed addresses are RVAs, not preferred virtual addresses.

| Hook/function | Upstream 1.1.1.9 RVA | 1.1.1.13 RVA | Evidence |
|---|---:|---:|---|
| Building simulation dispatcher | `0x139A70` | `0x139A70` | Expected 19-byte prologue is present at this exact entry |
| Construction tool frame | `0x2F0F10` | `0x2F0E90` | Expected 14-byte prologue found at this entry; one match in the executable |
| Factory connection query | `0x422C30` | `0x422BC0` | Expected 14-byte prologue found at this entry; one match in the executable |
| Resource transfer | `0x420350` | `0x420290` | Expected 17-byte prologue found at this entry; one match in the executable |

These three changed RVAs have been updated on branch `adaptation/1.1.1.13`. The runtime byte checks in the source remain enabled.

The dispatcher prologue also appears elsewhere in the executable, so its address is validated by the exact entry and function boundary, not by claiming that the byte pattern is globally unique.

## C3DDLL64 helper entries

The supplied DLL's COFF disassembly identifies the expected named functions at the original RVAs:

- `0x3E350`: `Collision2DBBox`
- `0x3A050`: `GetRotationStepFromRotation`
- `0xA1300`: `C3DVec3TransformCoord`

The helper RVAs currently remain unchanged.

## Addresses not yet safe to update

These locations require semantic identification before any hook or function pointer is changed:

- Generic storage UI: upstream RVA `0x706C00` is not a function entry in the supplied executable; it falls inside the runtime-function range `0x705380–0x706D92`. The short prologue pattern used by the upstream source occurs many times and is not sufficient to identify the replacement. The tempting `0x705380` entry is specifically rejected: a direct caller at `0x71E8DB` consumes its return value from `XMM0` as a float, which is incompatible with the upstream `void` generic-storage-UI function signature. It must not be hooked as the UI routine.
- Aggregate storage touch: upstream RVA `0x17AF70` is not a function entry in the supplied executable; it falls inside `0x17AF2F–0x17AF8F`. The correct corresponding function still needs to be identified.
- Building highlight: upstream RVA `0x40BD60` falls inside `0x40BCA0–0x40C34D`, rather than at a function entry. Its replacement must be mapped by call-site/semantic analysis.
- Vector grow helper: `0xB1700` is a runtime-function entry (`0xB1700–0xB1782`). Its disassembly reads the vector begin/end/capacity fields, compares available pointer slots with the requested additional element count, computes a growth capacity, and tail-calls the allocation/growth routine. This matches the source call shape `VectorGrowU64Fn(vectorBeginField, extraElements)`; it is now considered semantically confirmed for the pointer-vector growth use.

## Fail-closed safety change

Commit `7f55bb4174d41c7b6ea09e90cc7898c07d5aedcb` sets the aggregate-storage refresh and native building-highlight function pointers to null on 1.1.1.13. The old RVAs are inside runtime functions, not validated entries; calling them as functions could execute from the middle of unrelated instructions. With these helpers disabled, aggregate visual refresh and the optional green placement highlight are unavailable, but the code no longer calls those two unverified addresses.

The generic storage UI hook is a separate blocker: the old RVA `0x706C00` does not match the expected entry prologue in 1.1.1.13. The current hook admission logic fails closed and returns plugin-start failure rather than installing a speculative hook. A nearby runtime-function start at `0x705380` exists, but its semantic equivalence has not yet been proven; it is not substituted merely because the prologue matches.

## Important remaining work

Updating function RVAs is only part of the port. The source also contains many hard-coded 1.1.1.9 object-layout offsets for game globals, buildings, templates, storage descriptors, and C3D structures. Those layouts must be independently checked against 1.1.1.13 before enabling the corresponding runtime features.

No claim of full compatibility or in-game stability is made at this stage. The mod must not be released until all hooks and layouts are verified, the DLL builds, and the game is tested.


## Build validation (candidate only)

A Windows GitHub Actions build was added on the adaptation branch. The run completed successfully after removing two obsolete, unreferenced helper-RVA constants that were rejected by the project's `/W4 /WX` warning policy.

- Workflow run: [45493246537](https://github.com/Blakosta/WRSR-EnhancedStorageManagement-1.1.1.13/actions/runs/45493246537)
- Build result: C++ compilation passed; RML manifest verification passed; PE verification passed; artifact upload passed.
- Candidate DLL SHA-256: `b73cdc6723fc0f91df04582a3f22689ee7927dcec7e84b506f06d1dc0909b7be`
- Artifact: `EnhancedStorageManagement-WRSR-1.1.1.13-candidate`

This is only a reproducible build/package check. It does **not** establish runtime compatibility. The manifest still advertises 1.1.1.9 only, and the generic-storage UI hook still fails closed on 1.1.1.13 until its replacement entry is semantically identified. The object layouts also remain unverified. Do not install this artifact as a 1.1.1.13 release.
