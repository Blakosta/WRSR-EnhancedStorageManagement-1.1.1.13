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

- Aggregate storage touch: upstream RVA `0x17AF70` is not a function entry in the supplied executable; it falls inside `0x17AF2F–0x17AF8F`. The correct corresponding function still needs to be identified.
- Building highlight: upstream RVA `0x40BD60` falls inside `0x40BCA0–0x40C34D`, rather than at a function entry. Its replacement must be mapped by call-site/semantic analysis.
- Vector grow helper: `0xB1700` is a runtime-function entry (`0xB1700–0xB1782`). Its disassembly reads the vector begin/end/capacity fields, compares available pointer slots with the requested additional element count, computes a growth capacity, and tail-calls the allocation/growth routine. This matches the source call shape `VectorGrowU64Fn(vectorBeginField, extraElements)`; it is now considered semantically confirmed for the pointer-vector growth use.

## Fail-closed safety change

Commit `7f55bb4174d41c7b6ea09e90cc7898c07d5aedcb` sets the aggregate-storage refresh and native building-highlight function pointers to null on 1.1.1.13. The old RVAs are inside runtime functions, not validated entries; calling them as functions could execute from the middle of unrelated instructions. With these helpers disabled, aggregate visual refresh and the optional green placement highlight are unavailable, but the code no longer calls those two unverified addresses.

The generic storage UI blocker was subsequently resolved by the ABI-based mapping documented below: source RVA `0x706DA0` now points to the matching runtime-function entry. The rejected `0x705380` candidate remains explicitly excluded.

## Important remaining work

Updating function RVAs is only part of the port. Several important 1.1.1.13 layouts are now independently corroborated: the global root slot and C3D manager field, building-list/template/completion fields, live building storage vector and descriptor stride, template storage-blueprint vector, storage transport/role/capacity/block flags, and C3D type/instance/mesh geometry fields. Remaining offsets still requiring field-level checks include the allocation-vector end pointer at descriptor `+0x20`, building position fields, and any other inherited offsets used by active code. The allocation-vector begin pointer, stride, weight/active fields, and resource coefficient table are now corroborated. The building instance ID at `+0x5F4` and template collision type at `+0x410` are now verified.

No claim of full compatibility or in-game stability is made at this stage. The mod must not be released until all hooks and layouts are verified, the DLL builds, and the game is tested.


## Build validation (candidate only)

A Windows GitHub Actions build was added on the adaptation branch. The run completed successfully after removing two obsolete, unreferenced helper-RVA constants that were rejected by the project's `/W4 /WX` warning policy.

- Workflow run: [45493246537](https://github.com/Blakosta/WRSR-EnhancedStorageManagement-1.1.1.13/actions/runs/45493246537)
- Build result: C++ compilation passed; RML manifest verification passed; PE verification passed; artifact upload passed.
- Candidate DLL SHA-256: `b73cdc6723fc0f91df04582a3f22689ee7927dcec7e84b506f06d1dc0909b7be`
- Artifact: `EnhancedStorageManagement-WRSR-1.1.1.13-candidate`

This is only a reproducible build/package check. It does **not** establish runtime compatibility. The manifest still advertises 1.1.1.9 only, and the generic-storage UI hook still fails closed on 1.1.1.13 until its replacement entry is semantically identified. The object layouts also remain unverified. Do not install this artifact as a 1.1.1.13 release.

## Global root-slot check

The source's `RVA_GAME_ROOT_SLOT = 0x9941F0` has been checked against the 1.1.1.13 PE layout and code references:

- The RVA falls inside the executable's `.data` virtual range (`0x992000–0xA56718`). The section has a small on-disk raw payload but a much larger virtual size, so the address is part of the mapped zero-initialized data region; the short raw size reported by section tools is not evidence that the slot is outside the image.
- The disassembly contains numerous RIP-relative references resolving exactly to VA `0x1409941F0` (for example at `0x140005976`, `0x140007C3F`, `0x140007D2D`, and `0x14000836F`). These references confirm that the address remains an actively used global pointer slot in this executable build.
- This validates the global slot's image location and continued use, but does **not** independently validate every object-member offset reached through the pointer. In particular, `GAME_C3D_BUILDING_MANAGER = 0xFF8` and all building/template/storage layouts remain separate checks.

This is an additional static-analysis result only; it does not remove the generic-storage UI blocker or establish in-game compatibility.

## Generic storage UI hook: semantic mapping found

The previous blocker at the old RVA `0x706C00` is now resolved for static mapping. The replacement selected in the source is `0x706DA0`:

- `0x706DA0` is an exact runtime-function entry in the 1.1.1.13 `.pdata` map, with the expected 15-byte prologue `48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57`.
- A direct caller at VA `0x14071E857` sets `RCX`, `RDX`, `R8`, and `R9`, then populates all twelve stack argument slots from `[rsp+0x20]` through `[rsp+0x78]` before calling `0x140706DA0`.
- The caller's argument arrangement matches the upstream `GenericStorageUiFn` ABI: four register arguments followed by twelve stack arguments, including pointer/float/integer/byte positions in the expected order. This is much stronger evidence than a matching prologue alone and distinguishes it from the rejected `0x705380` function, whose caller consumes a float return value.
- The source constant `RVA_GENERIC_STORAGE_UI` has therefore been changed to `0x00706DA0`; the existing runtime prologue check remains enabled.

This is a semantically supported static mapping, but it still requires a fresh CI build and in-game validation. Other object layouts and the aggregate-refresh/building-highlight helpers remain unresolved.

## Rebuild after UI-hook mapping

The source change to `RVA_GENERIC_STORAGE_UI = 0x00706DA0` passed the Windows CI build:

- Workflow run: [45494164122](https://github.com/Blakosta/WRSR-EnhancedStorageManagement-1.1.1.13/actions/runs/45494164122) — success.
- C++ build, package verification, and artifact upload all passed.
- Candidate artifact ID: `14966508623` (`EnhancedStorageManagement-WRSR-1.1.1.13-candidate`).
- Artifact digest: `sha256:fe50934c9fc8b7c737f1d0d00710654b3b82dfe570d41ecd69cc8826fc7f2b98`.

The candidate is still not a release: this CI run proves compilation/package checks only. The game manifest and the remaining object-layout/helper validations plus an in-game test are still outstanding.

## C3D building-manager pointer field

One object-layout field has now gained independent 1.1.1.13 evidence:

- The executable constructor reads the same global root slot at VA `0x1409941F0`, allocates an object of exactly `0xE870` bytes, constructs it, and stores the resulting pointer at root-object offset `+0xFF8` (sequence around `0x140006E79–0x140006E92`).
- The corresponding teardown path later reads root `+0xFF8`, destroys the object, and frees exactly `0xE870` bytes (around `0x140008CEC–0x140008D1B`).
- This is consistent with the source's C3D building-manager interpretation: its known `types` vector fields at `+0xE718/+0xE720` fit within that `0xE870` allocation. The global root slot and `GAME_C3D_BUILDING_MANAGER = 0xFF8` therefore have strong constructor/destructor corroboration on 1.1.1.13.

This does not validate the other game/building/template/storage offsets. Those remain a separate reverse-engineering task.

## Live building storage-vector layout

The generic storage UI caller at VA `0x14071E857` also independently corroborates the live building storage-vector layout used by the mod:

- It reads vector begin/end from building offsets `+0x970` and `+0x978`.
- It subtracts begin from end and computes the element count using the reciprocal-multiply sequence for division by `0xE0` (`0x4924924924924925`, followed by a six-bit shift and sign correction).
- It then iterates those elements and passes each one into the generic storage UI routine. This supports both `BUILDING_STORES_BEGIN = 0x970`, `BUILDING_STORES_END = 0x978`, and `STORE_STRIDE = 0xE0` on 1.1.1.13.

These three live-storage layout values are now corroborated by a real caller path. The contents/field offsets inside each `0xE0` descriptor (record vectors, capacity, transport, roles, flags) still need individual checks.

## Building-list and core building/template fields

The verified building-simulation dispatcher at RVA `0x139A70` provides direct field-level corroboration for several upstream offsets:

- It reads the game building-pointer vector begin/end at `game + 0x11B08` and `game + 0x11B10`, computes the count from 8-byte pointers, and iterates the entries.
- For each building pointer it checks the template pointer at `building + 0x318`, then reads the live template runtime type at `template + 0x360`.
- It reads the building completion float at `building + 0x604` during its eligibility checks.

These observations support `GAME_BUILDINGS_BEGIN = 0x11B08`, `GAME_BUILDINGS_END = 0x11B10`, `BUILDING_TEMPLATE = 0x318`, `TEMPLATE_RUNTIME_TYPE = 0x360`, and `BUILDING_COMPLETION = 0x604` on 1.1.1.13. They are direct reads in the dispatcher rather than assumptions based on the old build.

## C3DDLL geometry/type layout recheck

The supplied `C3DDLL64.dll` 1.1.1.13 binary independently confirms the C3D layout used by the placement-footprint code. In `Collision2DBBox` (RVA `0x3E350`):

- The type array is addressed through manager `+0xE718`, with each type selected using stride `0x2330`.
- The live instance array is read from type `+0x2C0`; an instance index is scaled by `0x118` before indexing it. The adjacent vector end field is `+0x2C8` in the manager's type methods.
- Instance position fields are read at `+0x10`, `+0x14`, and `+0x18`; the discrete rotation step is read at `+0x1C`.
- The type's primary mesh pointer is read at `+0x58`, secondary mesh at `+0xA0`, and flip-Z flag at `+0x45F`.
- Mesh bounds are read at `+0x74`, `+0x7C`, `+0x80`, and `+0x88`.

This corroborates `C3D_TYPES_BEGIN`, `C3D_TYPE_STRIDE`, `C3D_TYPE_INSTANCES_BEGIN/END`, `C3D_INSTANCE_STRIDE`, the instance position/rotation offsets, both mesh-pointer offsets, `C3D_TYPE_FLIP_Z`, and the mesh bounding-box offsets against the exact supplied DLL. The helper addresses previously verified in the same DLL remain unchanged.

## Template storage-descriptor layout

The 1.1.1.13 executable function at RVA `0x0DEB70` directly walks a building template's storage-blueprint vector:

- It reads vector begin/end at template offsets `+0x778` and `+0x780`.
- It computes the descriptor count using division by `0xE0`, then indexes descriptors at that stride.
- For each descriptor it reads transport at `+0x90` and role at `+0x94`.
- It reads the descriptor's resource-record vector begin/end at `+0x00/+0x08`; the size check and count logic use `0x10`-byte records.

This independently supports `TEMPLATE_STORES_BEGIN/END`, `STORE_STRIDE`, `STORE_TRANSPORT`, `STORE_ROLE`, `STORE_RECORDS_BEGIN/END`, and `RECORD_STRIDE` on 1.1.1.13. The descriptor capacity and block-input/output flag offsets still need their own field-level checks.

## Live storage-descriptor fields

A later section of the 1.1.1.13 template/storage processing routine at RVA `0x0DECC0` iterates the same `0xE0` descriptors and directly checks their fields:

- `descriptor + 0x94` is read as the role and compared against role codes.
- `descriptor + 0x90` is compared against the requested transport.
- `descriptor + 0x85` and `descriptor + 0x84` are read as byte flags and compared independently.
- `descriptor + 0x8C` is read as a float and compared against a requested capacity value.
- The resource-record vector at `+0x00/+0x08` is measured in `0x10`-byte records.

This confirms the source offsets `STORE_ROLE`, `STORE_TRANSPORT`, `STORE_BLOCK_INPUT`, `STORE_BLOCK_OUTPUT`, and `STORE_CAPACITY` against the 1.1.1.13 executable. The semantic naming of the two byte flags follows the upstream storage logic; their exact polarity should still be checked in-game before changing behavior.

## Construction-tool and cursor layout

The 1.1.1.13 construction-tool frame at RVA `0x2F0E90` and tool-catalog code provide direct evidence for the following game/tool fields:

- The current tool pointer is read from `game + 0xD428`; its template pointer is read at `tool + 0x48`.
- The tool catalog vector uses begin/end at `game + 0xD280/+0xD288`. Its element count is computed by division by `0x2D0`, and the loop indexes entries with stride `0x2D0`.
- The cursor position is written/read as X/Y at `game + 0xF6C/+0xF70` and Z at `game + 0xF74`.
- Cursor rotation is read and updated at `game + 0xF94`.

This corroborates `GAME_CURRENT_TOOL`, `TOOL_TEMPLATE`, `GAME_TOOLS_BEGIN/END`, `TOOL_STRIDE`, `GAME_CURSOR_X/Y/Z`, and `GAME_CURSOR_ROTATION` on 1.1.1.13.

## Recipe vectors: separate input/output rate fields

The building simulation/production routine at RVA `0x177010` directly verifies the recipe layout on 1.1.1.13:

- The building's template pointer is read at building `+0x318`.
- Input/consumption vector begin/end are template `+0x688/+0x690`. The routine divides the byte length by `0x10`, indexes records with stride `0x10`, reads the resource pointer at record `+0x00`, and reads its rate at record `+0x08`.
- Output/production vector begin/end are template `+0x6B8/+0x6C0`. It also divides by `0x10`, indexes at stride `0x10`, reads the resource pointer at `+0x00`, but reads the rate at record `+0x0C`.

This confirms both vector offsets and `RECIPE_STRIDE = 0x10`, while revealing that the input and output rate fields are not interchangeable. The source now defines `RECIPE_INPUT_RATE = 0x08` and `RECIPE_OUTPUT_RATE = 0x0C`. Its resource-discovery paths do not request rates, so they now validate the resource pointer without accidentally treating the other record field as a rate.

## Resource-transfer path corroboration

The resource-transfer function at RVA `0x420290` calls the building-storage processing routine at `0x4243C0`. That routine independently walks live building stores using `+0x970/+0x978` and stride `0xE0`, reads each store's transport at `+0x90`, and walks the store's resource records through descriptor `+0x00/+0x08` with `0x10`-byte entries. It also checks the template's input and output recipe vectors at `+0x688/+0x690` and `+0x6B8/+0x6C0`, dividing both byte lengths by `0x10` and comparing each record's resource pointer.

This provides a second independent caller path for the live storage vector, descriptor transport, record vector, and both recipe-vector layouts. It does not yet establish the allocation vector at descriptor `+0x18/+0x20`, its element fields, or the resource transport coefficient table.

## Building instance identity and collision type

The 1.1.1.13 building collision/selection path at VA `0x14013819B` reads the building's template pointer at `building + 0x318`, the building instance ID at `building + 0x5F4`, and the template collision type at `template + 0x410`. It then loads the global root slot at `0x1409941F0`, follows root `+0xFF8` to the C3D building manager, and passes the collision type and instance ID into the manager's collision/lookup methods. The same field reads are repeated in the paired lookup call at `0x1401381BF–0x1401381E1`.

This directly corroborates `BUILDING_TEMPLATE = 0x318`, `BUILDING_INSTANCE_ID = 0x5F4`, `TEMPLATE_COLLISION_TYPE = 0x410`, `GAME_ROOT_SLOT = 0x9941F0`, and `GAME_C3D_BUILDING_MANAGER = 0xFF8` for the C3D identity path on 1.1.1.13. It does not independently validate the separate `BUILDING_POSITION_X/Y/Z` offsets used by the mod's proximity calculations.

## Allocation records and resource coefficients

The 1.1.1.13 resource-transfer path at RVA `0x4243C0` directly reads the allocation-vector pointer at live store descriptor `+0x18`. It indexes allocation records with a `0x10`-byte stride (the loop index is doubled and then scaled by eight), reading the allocation weight as a float at record `+0x08` and the active flag at `+0x0C`. This corroborates `STORE_ALLOC_BEGIN = 0x18`, `ALLOC_STRIDE = 0x10`, `ALLOC_WEIGHT = 0x08`, and `ALLOC_ACTIVE = 0x0C`.

The same routine reads the resource coefficient using `resource + 0xCC + transport * 0x20` (instructions around `0x424C7C` and `0x424CFD`) and multiplies it by the store capacity at descriptor `+0x8C`. This independently confirms `RESOURCE_COEFF_BASE = 0xCC`, `RESOURCE_COEFF_STRIDE = 0x20`, `STORE_CAPACITY = 0x8C`, and the transport field at `+0x90`.

The allocation vector's end pointer at descriptor `+0x20` is still not independently identified in this specific routine; its source constant remains an inherited value pending a direct reference check.

## C3D connection-node metadata: executable evidence

The connection-node mapping is based on the **game executable** `SOVIET64.exe`, not `C3DDLL64.dll`. The symbol-style address `FUN_1400BFF40` refers to VA `0x1400BFF40` in the executable (RVA `0x0BFF40`). Its disassembly begins with a function prologue at that address and iterates the pointer vector at object `+0x00/+0x08`. For each node pointer it reads the connection-metadata pointer at node `+0xE0`, then reads the native connection-kind code at metadata `+0x20`. The observed call sites in this function compare the code against values including `0x02`, `0x12`, `0x17`, `0x18`, and transport-related ranges; the source's specific 3/4, 5/6, 0x13/0x14, and 0x19/0x1A mapping still needs to be treated as a semantic mapping inferred from those comparisons and validated against live buildings.

A separate disassembly at the same numerical RVA in `C3DDLL64.dll` resolves to an unrelated function because the DLL has a different image base and address space. That DLL observation does not invalidate the executable's `FUN_1400BFF40` evidence. The source node metadata path is therefore retained; this note corrects the binary attribution and records the distinction.
