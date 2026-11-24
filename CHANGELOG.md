# Changelog

## 1.0.1

Compatibility and placement-feedback patch.

- Restored WRSR's native whole-building green construction highlight path.
- Removed the cube/marker placement fallback and the obsolete separate render hook/highlight thunk.
- Stabilized road/rail freight gateway attachment: transient live routing-resource records no longer gate or invalidate a geometrically valid delivery-to-pool relationship.
- Added RML `loadAfter` ordering for Dock Distribution Office, ForceNodes and Parallel Utilities.
- Added validated pre-existing `FF 25` detour admission so ESM can wrap the current effective hook owner instead of requiring untouched vanilla entry bytes.
- Verified the critical simulation chain as `ESM → Dock Distribution Office → WRSR` in a full RML stack.
- Verified the construction chain as `ESM → Parallel Utilities → ForceNodes → WRSR` in the same session.
- No storage-pooling model or save-format changes.

## 1.0.0

Initial public release.

- Unified compatible storage capacity across resource/production roots and attached real storage buildings.
- Road/rail freight delivery buildings can access compatible pools directly or through an attached storage surface.
- Unified native ingress/egress transfer bridge for stations, factory connections and conveyors.
- Freight delivery routing buffers are access-only and add no logical capacity.
- Native storage allocation percentages and authored capacities remain authoritative.
- Station connection-status UI understands proven virtual pool roots without persistent synthetic factory connections.
- Exact rotated-footprint link discovery with configurable edge gap.
- Green placement feedback for valid nearby relationships.
- Conservative three-class graph prevents recursive storage, delivery or resource-root chains.
- Exact-build hook validation, mediated hook chaining, conventional native PE hardening and reproducible source build.
