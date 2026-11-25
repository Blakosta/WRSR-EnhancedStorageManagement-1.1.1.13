# Binary analysis: WRSR 1.2.0.8

This is an initial static-analysis record for the exact binaries supplied for the 1.2.0.8 port. It is not a release certification and does not establish in-game stability.

## Input binaries

- `SOVIET64.exe` SHA-256: `91a2ff66fd02e02a51f38f4d2106ccb56af9510af9a6fb7e7b93ae4249358ea7`
- `C3DDLL64.dll` SHA-256: `331895c798092c1c24d8d7586631d8c514bcb0070dcb0d189eca7cf29ba8a483`
- Both are AMD64 PE32+ images. Their PE timestamps are 2026-11-20 18:55:46 UTC (EXE) and 2026-11-20 13:32:07 UTC (C3DDLL).

## Executable hook entries mapped

All addresses below are RVAs, not preferred virtual addresses.

| Hook | 1.2.0.8 RVA | Evidence |
|---|---:|---|
| Building simulation dispatcher | `0x152520` | Exact 19-byte dispatcher signature at this runtime-function entry. Direct call from simulation tick at `0x32AE63`; function resets and repopulates the game building-pointer vector and reads building template at `+0x318`. The same prologue also occurs at `0x4C7480`, so selection is based on caller/body semantics, not global uniqueness. |
| Construction tool frame | `0x30DBC0` | Exact 14-byte expected entry signature at a runtime-function boundary; one match in the executable. |
| Factory connection query | `0x446B40` | Exact 14-byte expected entry signature at a runtime-function boundary; one match. It resets temporary vectors at game offsets `+0x14528` and `+0x14540`, and checks the queried building's completion at `+0x604`. |
| Resource transfer | `0x444210` | Exact 17-byte expected entry signature at a runtime-function boundary; one match. The function has split unwind/runtime ranges: the entry guard occupies `0x444210–0x44423A`, and execution continues in the following range. It checks the resource sentinel at `resource+0x44`; the continued body checks building completion at `+0x604`. |
| Generic storage UI | `0x73CA40` | The expected 15-byte prologue is common and not unique. Semantic caller evidence: a caller at `0x746A35` iterates 0xE0-byte storage descriptors through the building's vector at `+0x970`, reads descriptor transport/capacity/block fields, and calls this function with the 16-argument ABI used by the plugin. The function has 47 direct callers. |

## C3DDLL64 helper exports

The DLL export table identifies the exact functions and their current RVAs:

- `0x3E410`: `Collision2DBBox@C3D_BUILDING`
- `0x3A0E0`: `GetRotationStepFromRotation@C3D_BUILDING`
- `0xA13E0`: `C3DVec3TransformCoord`

These are exported function entries, not guesses based on nearby code. The former 1.1.1.13 RVAs now land inside other functions and must not be reused.

## Layout fields corroborated in 1.2.0.8

- Game building-pointer vector: begin/end at game `+0x11E70/+0x11E78). The dispatcher clears this vector, then appends building pointers; a consumer iterates its 8-byte elements. The appended elements are used as building objects and their template pointer is read at `building+0x318`.
- Building template pointer: `building+0x318`.
- Building completion: `building+0x604`.
- Live building storage vector: `building+0x970/+0x978`, descriptors have stride `0xE0`.
- Storage descriptor fields observed by the UI path: transport `+0x90`, capacity `+0x8C`, byte flags `+0x84/+0x85).
- Template recipe vectors: input `+0x688/+0x690), output `+0x6B8/+0x6C0); records are 0x10 bytes.
- Template storage-blueprint vector: `+0x778/+0x780); descriptors use the same 0xE0 stride and resource records use 0x10-byte elements.
- C3D manager type vector begins at manager `+0xE718); type stride `0x2330). Collision code uses type instances at `type+0x2C0), instance stride `0x118), mesh pointers at `type+0x58/+0xA0), and flip-Z at `type+0x45F). Rotation-step count remains at manager `+0x48).
- Root pointer slot: executable RVA `0x9DDC88). The initialization path loads the root object from this slot, allocates a `0xE870)-byte C3D manager, and stores it at root `+0x1098); teardown reads and frees the same object/size. The C3D type-vector fields fit within this allocation.
- Temporary connection query vector 1: game `+0x14528/+0x14530) is reset by the query function and consumed by downstream query/UI paths. `+0x14538) is the inferred capacity field from the three-pointer vector layout; it has not yet been independently read in the inspected paths. A second vector begins at `+0x14540).

## Intentionally disabled pending mapping

- Native building-highlight function and its colour-selector semantics. The old highlight RVA/signature does not match the new executable; source constant is set to zero, so the placement overlay fails closed.
- Native vector-growth helper. The old `0xB1700` address is inside an unrelated function in 1.2.0.8. Source constant is set to zero, so the optional freight connection-warning UI bridge remains disabled rather than calling an unverified function.
- Aggregate-storage visual refresh remains disabled, as it was not mapped.

## Remaining field-level work

- Current construction-tool pointer, tool catalog, cursor position and cursor rotation offsets.
- Allocation-vector begin/end and allocation record fields; resource coefficient table layout.
- Exact resource-transfer downstream routine and any changed internal storage offsets not covered above.
- Native building-highlight entry, caller ABI, and the native selector value for green.
- Native vector-growth helper and the second temporary connection vector's role.
- Any remaining object fields used by the port that are still inherited from 1.1.1.13.

A successful compilation and an in-game test are still required. No compatibility or release claim is made by this document.
