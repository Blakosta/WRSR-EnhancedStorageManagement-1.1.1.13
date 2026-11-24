# Enhanced Storage Management 1.0.1 — Regression Checklist

The 1.0.1 patch does not change the storage-pool data model. Its release candidate passed the full-stack smoke test described below on WRSR 1.1.1.9.

## Placement feedback

- Select a compatible storage, road/rail freight station, loader or unloader.
- Move it within the configured link gap of a compatible existing building.
- The existing building itself should be tinted green by WRSR's native whole-building highlight.
- No cube/marker fallback exists in 1.0.1.

**Release result:** passed by manual gameplay observation.

## Freight-gateway spatial stability

- Move a road cargo station progressively from outside range toward a compatible factory/storage pool.
- Once geometry enters the valid gap, the relationship should not disappear merely because the station's live routing-resource vector changes.
- Repeat with an empty station and while cargo is flowing where practical.

**Release result:** the revised relationship logic worked in the final smoke session and no structural detach/relink failure was observed.

## Storage regression

- Verify station/factory/storage unified ingress and egress.
- Verify native storage allocation percentages remain unchanged.
- Save/reload and confirm pool relationships rediscover normally.

**Release result:** core ESM behavior was reported working in the final full-plugin-stack session. The patch did not change the pool persistence model.

## Full RML compatibility stack

Enable Dock Distribution Office, ForceNodes and Parallel Utilities together with Enhanced Storage Management using RML automatic ordering.

Expected critical simulation chain:

`WRSR → Enhanced Storage Management → Dock Distribution Office → original WRSR continuation`

Expected construction chain:

`WRSR → Enhanced Storage Management → Parallel Utilities → ForceNodes → original WRSR continuation`

**Release result:** passed. Both chains were recorded by RML as `CURRENT VERIFIED CHAIN / PASS` and remained intact for the full diagnostic run.

See `RUNTIME_VALIDATION_v1.0.1.md` for the captured release-validation summary.
