# Runtime Validation — Enhanced Storage Management 1.0.1

## Environment

- WRSR: 1.1.1.9
- Republic Mod Loader: 1.0.1
- Enhanced Storage Management DLL SHA-256: `2c1b12740d362c1ad3b4b59fd93e0754c18289035ceac48b1a58938cbfb0c183`
- Validation profile: RML Detailed Diagnostic / deep-test
- Session date: 2026-08-31
- Runtime duration: approximately 9 minutes 16 seconds
- Process exit: normal (`0x0`)

The active stack included Dock Distribution Office, ForceNodes, Parallel Utilities and numerous other runtime plugins.

## Ordering proof

RML honored ESM's `loadAfter` declarations even though the user's previously stored ESM priority remained 250. The observed relevant initialization order was:

1. Dock Distribution Office
2. ForceNodes
3. Parallel Utilities
4. Enhanced Storage Management

The manifest ordering edges are therefore effective without requiring the user to manually reset an old saved priority.

## Critical simulation chain

Target: `SOVIET64.exe+0x139A70`

Observed chain:

`WRSR entry → Enhanced Storage Management → Dock Distribution Office → original WRSR trampoline`

RML result:

- integrity: `PASS`
- proof: `CURRENT VERIFIED CHAIN`
- ESM execution order: 0
- DDO execution order: 1

This directly verifies the 1.0.1 fix for the 1.0.0 failure mode where DDO installed after ESM and made ESM unreachable.

## Construction/placement chain

Target: `SOVIET64.exe+0x2F0F10`

Observed chain:

`WRSR entry → Enhanced Storage Management → Parallel Utilities → ForceNodes → original WRSR trampoline`

RML result:

- integrity: `PASS`
- proof: `CURRENT VERIFIED CHAIN`
- ESM execution order: 0
- Parallel Utilities execution order: 1
- ForceNodes execution order: 2

The manual smoke test also reported the revised ESM behavior working in-game.

## ESM-exclusive hooks

The following ESM targets remained healthy and directly owned throughout the session:

- `0x420350` — unified resource transfer
- `0x706C00` — generic storage UI
- `0x422C30` — factory/storage connection query

All were recorded as `PASS / CURRENT VERIFIED CHAIN`.

## Long-run chain stability

RML diagnostic summary:

- host hook requests: 60
- runtime chains: 53 PASS / 1 recognized external / 0 broken
- shared chains: 6
- read-only monitor scans: 270
- late executable-change windows: 0
- final hook-chain audit issues: 0

No late plugin modification displaced an ESM hook during the session.

## Unrelated stack observations

The diagnostic session still attributed unmanaged/direct executable changes to:

- Cableway Customs House
- Dock Distribution Office (one unrelated page)
- Rolling Stock Road Transport

Temporary Track was marked degraded because one of its own hook requests was not installed. These observations did not produce a broken ESM chain and are not treated as ESM 1.0.1 release blockers.
