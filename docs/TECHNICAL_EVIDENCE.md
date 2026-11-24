# WRSR 1.1.1.9 Technical Evidence

The release implementation is grounded in the supplied WRSR 1.1.1.9 decompile and runtime diagnostics.

## Primary WRSR hooks

| Purpose | RVA | Release behavior |
| --- | ---: | --- |
| Building simulation dispatcher | `0x139A70` | Temporary logical-pool simulation window |
| Signed resource transfer | `0x420350` | Unified legitimate ingress/egress across root + real storages |
| Factory/storage temporary query | `0x422C30` | Adds proven virtual roots to station UI query only |
| Construction/tool frame | `0x2F0F10` | Placement target discovery; safely chains with mediated prior owner |
| Generic storage UI | `0x706C00` | Temporary pooled display shadow |

Supporting native WRSR routines:

- building highlight: `0x40BD60`;
- aggregate storage visual/material refresh: `0x17AF70`;
- temporary `vector<building*>` grow helper: `0x0B1700`;

## C3DDLL64 helpers

- `Collision2DBBox`: `0x3E350`;
- `GetRotationStepFromRotation`: `0x3A050`;
- `C3DVec3TransformCoord`: `0xA1300`.

These are used for actual rotated footprint separation and safe placement-preview geometry.

## Exact release entry validation

The release validates the exact 1.1.1.9 prologue bytes for each hook. It also accepts the standard RML-mediated 14-byte absolute-entry bridge (`FF 25 00 00 00 00 <abs64>`) when the destination is readable, allowing mediated chaining without a general-purpose instruction decoder.

Runtime diagnostics and the supplied executable confirm the exact render/input entry bytes:

`40 55 56 48 8D AC 24 98 E3 FF FF B8 68 1D 00 00`


## Resource-transfer semantics

The `0x420350` routine accepts a signed amount. Positive transfers add cargo; negative transfers remove cargo. WRSR reaches this primitive after higher-level station/factory-connection/conveyor logic has already selected a legitimate transfer context, making it the narrowest shared boundary at which the plugin can expose combined storage without bypassing WRSR's routing/authorization logic.
