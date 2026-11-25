# Binary analysis: WRSR 1.2.0.8

Initial static-analysis record for the exact binaries supplied for this port. This is not a release certification and does not establish in-game stability.

## Input binaries

- `SOVIET64.exe` SHA-256: `91a2ff66fd02e02a51f38f4d2106ccb56af9510af9a6fb7e7b93ae4249358ea7`
- `C3DDLL64.dll` SHA-256: `331895c798092c1c24d8d7586631d8c514bcb0070dcb0d189eca7cf29ba8a483`
- Both are AMD64 PE32+ images. PE timestamps: EXE `2026-11-20 18:55:46 UTC`; C3DDLL `2026-11-20 13:32:07 UTC`.

## Executable hook entries mapped

All addresses below are RVAs, not preferred virtual addresses.

| Hook | 1.2.0.8 RVA | Evidence |
|---|---:|---|
| Building simulation dispatcher | `0x152520` | Exact 19-byte expected signature at a runtime-function entry. Direct call from simulation tick at `0x32AE63`; function resets and repopulates the game building-pointer vector and reads the building template at `+0x318`. The same prologue also occurs at `0x4C7480`, so selection is based on caller/body semantics, not global uniqueness. |
| Construction tool frame | `0x30DBC0` | Exact 14-byte expected entry signature at a runtime-function boundary; one match in the executable. |
| Factory connection query | `0x446B40` | Exact 14-byte expected entry signature at a runtime-function boundary; one match. It resets temporary vectors at game offsets `+0x14528` and `+0x14540`, and checks the queried building's completion at `+0x604`. |
| Resource transfer | `0x444210` | Exact 17-byte expected entry signature at a runtime-function boundary; one match. The function has split unwind/runtime ranges: the entry guard occupies `0x444210–0x44423A`, and execution continues in the following range. It checks the resource sentinel at `resource+0x44`; the continued body checks building completion at `+0x604`. |
| Generic storage UI | `0x73CA40` | The expected 15-byte prologue is common and not unique. Semantic caller evidence: a caller at `0x746A35` iterates 0xE0-byte storage descriptors through the building vector at `+0x970`, reads descriptor transport/capacity/block fields, and calls this function with the 16-argument ABI used by the plugin. The function has 47 direct callers. |

## C3DDLL64 helper exports

The DLL export table identifies the exact functions and current RVAs:

- `0x3E410`: `Collision2DBBox@C3D_BUILDING`
- `0x3A0E0`: `GetRotationStepFromRotation@C3D_BUILDING`
- `0xA13E0`: `C3DVec3TransformCoord`

These are exported function entries, not guesses based on nearby code. The former 1.1.1.13 RVAs now land inside other functions and must not be reused.

## Layout fields corroborated in 1.2.0.8

- Game building-pointer vector: begin/end at game `+0x11E70/+0x11E78`. The dispatcher clears this vector, then appends building pointers; a consumer iterates its 8-byte elements. Each element is used as a building object and its template pointer is read at `building+0x318`.
- Building template pointer: `building+0x318`.
- Building completion: `building+0x604`.
- Building C3D instance ID: `building+0x5F4`.
- Live building storage vector: `building+0x970/+0x978`; descriptors have stride `0xE0`.
- Storage descriptor fields observed by the UI path: transport `+0x90`, capacity `+0x8C`, byte flags `+0x84/+0x85`.
- Runtime building class: template `+0x368`; executable comparisons confirm the existing storage/factory/mine values `5`, `6`, and `7`.
- C3D type index: template `+0x418`; the former `+0x410` value is stale.
- Template recipe vectors: input `+0x688/+0x690`, output `+0x6B8/+0x6C0`; records are 0x10 bytes.
- Template storage-blueprint vector: `+0x778/+0x780`; descriptors use the same 0xE0 stride and resource records use 0x10-byte elements.
- C3D manager type vector: manager `+0xE718/+0xE720`, type stride `0x2330`.
- Each C3D type's instance vector: `type+0x2C0/+0x2C8`, instance stride `0x118`. Collision code reads world position at `instance+0x10/+0x14/+0x18`; the port now uses these records for building-to-building and cursor-to-building distance checks instead of stale direct building offsets.
- C3D type mesh pointers: `type+0x58/+0xA0`; flip-Z flag: `type+0x45F`; rotation-step count: manager `+0x48`.
- Root pointer slot: executable RVA `0x9DDC88`. Initialization loads the root object from this slot, allocates a `0xE870`-byte C3D manager, and stores it at root `+0x1098`; teardown reads and frees the same object/size.
- Current construction-tool pointer: game `+0xD4C0`.
- Tool catalog vector: game `+0xD280/+0xD288`.
- Cursor position: game `+0xF90/+0xF94/+0xF98`, read and updated by the construction-tool frame.
- Temporary connection query vector 1: game `+0x14528/+0x14530` is reset by the query function and consumed by downstream query/UI paths. `+0x14538` is the inferred capacity field from the three-pointer vector layout; a second vector begins at `+0x14540`.

## Native temporary-vector append

The old 1.1.1.13 vector-growth RVA `0xB1700` is inside an unrelated function in 1.2.0.8. The port instead uses the native 8-byte `std::vector::push_back` entry at executable RVA `0x54960`. Its 14-byte prologue is checked at runtime. It copies a pointer value, updates begin/end/capacity, and calls the game's native growth routine when needed. This helper is used by the game's own building dispatcher to append building pointers.

## Intentionally disabled pending mapping

- Native building-highlight candidate at `0x578290` has a matching runtime-function entry and building/template access pattern, but its ABI is not safe to call yet: the fifth stack argument is treated as a pointer, not the old plugin's integer flag. The `R9B` selector's colour mapping is unresolved. The candidate is not enabled.
- The game stores a 3-float cursor orientation vector starting at `+0xF9C`; the yaw component has not yet been identified. Placement overlay is therefore disabled along with the native highlight.
- Aggregate-storage visual refresh remains disabled because its native entry has not been mapped.

## Remaining field-level work

- Cursor rotation component/yaw axis.
- The current-tool object's template member at `tool+0x48` has not yet been independently checked against 1.2.0.8.
- Allocation-vector begin/end and allocation record fields; resource coefficient table layout.
- Exact resource-transfer downstream routine and any changed internal storage offsets not covered above.
- The second temporary connection vector's role.
- Any remaining object fields used by the port that are still inherited from 1.1.1.13 and have not been independently checked.

A successful compilation and an in-game test are still required. No compatibility or release claim is made by this document.
