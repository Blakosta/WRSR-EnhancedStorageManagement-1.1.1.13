# Enhanced Storage Management

Enhanced Storage Management is a native runtime mod for **Workers & Resources: Soviet Republic 1.1.1.9**. It allows compatible nearby storage and resource/production buildings to behave as one logical storage pool, while compatible road/rail freight delivery buildings can access that pool without writing synthetic factory connections or permanent pooled capacities into the save.

## Core behaviour

- Compatible **storage ↔ resource/production** relationships extend the relevant input/output pool.
- Compatible **freight delivery ↔ storage** and **freight delivery ↔ resource/production** relationships provide logistics access.
- A freight delivery building touching storage already attached to a resource building resolves to the same logical pool.
- Delivery buildings are **access ports, not extra storage**; their native routing buffers contribute zero logical pool capacity.
- Legitimate WRSR transfers through freight stations, factory connections and conveyors can fill or drain the compatible logical pool instead of stopping at the root building's authored buffer.
- Native storage allocation percentages remain authoritative.
- Storage↔storage, delivery↔delivery and resource↔resource propagation are intentionally not introduced.

## Version 1.0.1

Version 1.0.1 is a compatibility and placement-feedback patch over the initial release. It:

- restores WRSR's native whole-building green placement highlight and removes the cube/marker fallback;
- stabilizes road/rail freight gateway attachment so transient routing-resource records cannot invalidate a geometrically valid relationship;
- adds explicit RML ordering for Dock Distribution Office, ForceNodes and Parallel Utilities;
- chains over a validated existing `FF 25` entry detour instead of assuming the untouched WRSR entry is still exposed;
- preserves the 1.0.0 storage-pool data model and save behavior.

The final 1.0.1 build was runtime-validated in a full RML plugin stack on WRSR 1.1.1.9. The critical simulation chain was observed as `Enhanced Storage Management → Dock Distribution Office → WRSR`, while the construction chain was observed as `Enhanced Storage Management → Parallel Utilities → ForceNodes → WRSR`. Both remained healthy throughout the diagnostic session.

## Requirements

- Workers & Resources: Soviet Republic **1.1.1.9**.
- **Republic Mod Loader 1.0.1 or newer**.
- No WRSR DLC is required by the plugin itself.

The RML plugin ID is `ultimate-universe.enhanced-storage-management` and the runtime DLL is `EnhancedStorageManagement.dll`.

## Steam Workshop

https://steamcommunity.com/sharedfiles/filedetails/?id=3793101070

## Installation

### Steam Workshop

Subscribe to the Workshop item, enable the mod in Republic Mod Loader, and launch WRSR through RML.

### Manual / GitHub runtime package

Copy the contents of `ModFiles` (or the standalone runtime ZIP) into the WRSR/RML environment so that these files land under `rml/plugins`:

- `EnhancedStorageManagement.dll`
- `EnhancedStorageManagement.ini`
- `EnhancedStorageManagement.rml.json`

## Configuration

`EnhancedStorageManagement.ini` exposes two release settings:

- `LinkGapMeters=5` — maximum supported 2D edge-to-edge gap. Values are clamped to 2–20 metres.
- `ShowPlacementOverlay=1` — enables placement feedback. Compatible existing buildings use WRSR's native whole-building green highlight; no cube/marker fallback is used.

## Compatibility

RML automatic ordering is recommended. The 1.0.1 manifest declares non-mandatory `loadAfter` relationships for Dock Distribution Office, ForceNodes and Parallel Utilities because those plugins are known to share relevant WRSR entry points.

The hook admission logic itself is not hard-coded to those mods: a readable, validated standard `FF 25` entry detour can be wrapped so the previous effective owner remains reachable through the continuation chain. Arbitrary unmanaged overwrites performed after ESM installs cannot be guaranteed safe.

See `docs/COMPATIBILITY.md` and `docs/RUNTIME_VALIDATION_v1.0.1.md` for the verified full-stack chain.

## Save safety

The mod does not persist raw pointers, synthetic factory connections, or pooled-capacity values into WRSR saves. Pool capacity is exposed only in controlled simulation/transfer/UI windows and authored native capacity is restored before control returns to normal game state.

## Repository layout

- `ModFiles/` — exact player runtime payload for manual/RML installation.
- `Source/` — complete buildable plugin source, configuration/manifest templates and verification tools.
- `docs/` — architecture, compatibility, reverse-engineering evidence, testing and release verification notes.
- `CHANGELOG.md` — release history.
- `LICENSE` — GNU GPL v3 licence text.

Build from `Source/` with `build_release.bat` on Windows or `build_release.sh` in a compatible shell environment.

## Licence

Enhanced Storage Management is distributed under the GNU General Public License version 3. See `LICENSE`.

Workers & Resources: Soviet Republic, Republic Mod Loader, Steam, and their respective names and assets belong to their owners. This project is not affiliated with or endorsed by 3DIVISION, Hooded Horse, or Valve.
