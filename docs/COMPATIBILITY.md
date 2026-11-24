# Compatibility

## Supported game build

The 1.0.1 release targets **WRSR 1.1.1.9**. Hook entry bytes and relevant data layouts are validated against the supplied 1.1.1.9 executable/decompile before installation. Unknown entry states fail closed instead of being patched heuristically.

## Republic Mod Loader

The sidecar manifest uses the neutral plugin identity:

`ultimate-universe.enhanced-storage-management`

The DLL keeps the API 4 compatibility exports required by Republic Mod Loader's compatibility host for these native hooks. Users do not need a separate legacy loader installation. The public DLL/manifest identity remains the neutral `ultimate-universe.*` identity.


## Late-load shared-hook policy

Enhanced Storage Management intentionally prefers to install its shared hooks after known owners of the same WRSR entry points. The manifest declares `loadAfter` relationships for Dock Distribution Office, ForceNodes and Parallel Utilities and uses a late default priority for fresh RML configurations. Missing plugins do not create dependencies; the ordering edges apply only when those plugins are enabled.

This specifically resolves the observed Dock Distribution Office collision at `SOVIET64.exe+0x139A70`. In the failing 1.0.0 stack, ESM installed first and DDO later published its own FF25 detour on the same simulation dispatcher, leaving ESM unreachable. With the revised 1.0.1 order, DDO establishes its entry first and ESM then wraps the live validated detour. The intended execution path is therefore `WRSR -> ESM -> prior owner -> original WRSR`.

The same admission logic is generic for a readable standard FF25 entry detour, so ESM can chain over another compatible prior owner without knowing that plugin's implementation. This does not make arbitrary later unmanaged overwrites safe: a third-party plugin that ignores RML ordering/chaining and replaces the entry *after* ESM can still bypass earlier owners. RML automatic ordering is therefore the supported configuration.

## Shared hooks / ForceNodes

The construction-tool hook may already be owned by another plugin such as ForceNodes. Enhanced Storage Management accepts either:

- the exact validated WRSR 1.1.1.9 entry bytes; or
- RML's validated 14-byte absolute-entry bridge to a readable prior owner.

It then requests a mediated chain rather than overwriting the existing detour. The final 1.0.1 release-validation session observed the shared Enhanced Storage Management/Parallel Utilities/ForceNodes construction chain as healthy for the entire run, with ESM as the effective top owner.

The same exact-or-mediated-entry rule is used for other hook targets, so an existing standard RML-mediated bridge can be chained where the loader supports it.

## DLC independence

Core logic is data-driven from runtime WRSR building/storage/recipe structures and does not require a DLC asset, DLC building ID or entitlement check. Optional DLC/Workshop buildings can participate when their live descriptors satisfy the same compatibility rules; their absence does not affect plugin initialization.

## Other storage/logistics mods

The plugin intentionally avoids persistent ownership of WRSR storage capacity and factory-connection topology. A live root capacity that cannot be reconciled with the plugin's own transient shadow causes writes to be suspended rather than forcibly overwritten. This reduces the risk of fighting another mod that legitimately modifies the same store.

Unknown or ambiguous resource-lane selections fail conservatively back to native WRSR behavior.
