# Architecture

## Relationship model

Enhanced Storage Management uses three building classes:

1. **Storage** — compatible native storage descriptors that can contribute physical capacity.
2. **Resource root** — production, processing, service, utility, fuel, water or other supported buildings that own compatible native resource stores.
3. **Freight delivery** — road/rail cargo stations and loader/unloader gateways that expose compatible native routing descriptors.

Allowed edges are deliberately limited to:

- storage ↔ resource root;
- freight delivery ↔ storage;
- freight delivery ↔ resource root.

Storage↔storage, freight-delivery↔freight-delivery and resource-root↔resource-root propagation are blocked. This prevents recursive pool graphs and keeps one canonical resource root for a logical pool.

## Discovery and compatibility

The plugin does not use a hard-coded building-name allow-list. It derives compatibility from:

- WRSR runtime building class;
- live 0xE0 storage descriptors;
- transport class;
- recipe consumption/production resources;
- native allocation selectors where disambiguation is required;
- exact rotated building-footprint separation.

The configured `LinkGapMeters` is applied to minimum edge-to-edge footprint separation rather than centre distance.

Established relationships are maintained from structural evidence. Current cargo contents are allowed to help discover ambiguous relationships but are not permitted to tear down a structurally valid link. This is especially important for freight gateways whose routing records legitimately change as cargo changes.

## Capacity model

Only real storage buildings add logical capacity. Freight delivery buildings contribute zero capacity.

For each resource lane the plugin retains the root store's authored capacity and the compatible portion of each attached storage descriptor. Native multi-resource allocation percentages partition an attached storage's physical capacity; they are not rewritten by the plugin.

The root's live `STORE_CAPACITY` is never left permanently pooled. Instead, combined capacity is published only during controlled windows and restored to the authored value afterwards.

## Simulation window

The WRSR building-simulation dispatcher is wrapped so active logical pools are prepared immediately before native building simulation and published/redistributed immediately afterwards. This lets production and consumption logic see the combined pool while leaving the persistent native store layout unchanged.

## Unified transfer window

WRSR's signed resource-transfer primitive is wrapped for transfers that would otherwise hit the selected member/root's native capacity or stock boundary.

The bridge:

1. lets WRSR perform its normal legitimacy/routing checks;
2. resolves the selected station/storage member to a single proven pool/root for the resource;
3. opens a temporary combined-capacity/stock view only when native room/stock is insufficient;
4. calls the original WRSR transfer routine;
5. redistributes the resulting cargo across the physical root + attached storages;
6. restores authored capacities before returning.

The hook is recursion-guarded and bypasses ambiguous, unrelated, station-only and already-covered simulation-window transfers.

## Freight connection status

WRSR's station UI builds a temporary vector of connected factories/storages. Enhanced Storage Management augments only that temporary query result with proven logical pool roots. Persistent factory-connection topology is not modified.

## Storage UI

When a resource root has real attached storage, the generic storage UI receives a temporary shadow descriptor whose displayed capacity and cargo reflect the physical pool. The real store, resource records and allocation selectors are not rewritten for display.

Standalone storage ↔ station relationships remain on WRSR's native storage panel because the station contributes no capacity and must not alter player allocation percentages.

## Placement feedback

The construction hook discovers the compatible target after WRSR processes the current construction frame and calls WRSR's native whole-building green highlight directly. The previous frame's already-validated completed target is replayed before the next construction frame, avoiding pre-frame preview decoding. No cube/marker fallback is used.

## Save and object lifetime policy

- No raw pointers are serialized.
- No persistent synthetic factory connections are created.
- No synthetic storage descriptors are written into building definitions or saves.
- Authored capacities are treated as immutable reference values for transient pooling.
- Stale runtime relationships are discarded when building/store objects disappear or world vectors change.
