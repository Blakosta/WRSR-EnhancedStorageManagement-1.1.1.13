#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-$ROOT/out}"
CLANG_CL="${CLANG_CL:-clang-cl}"
LLD_LINK="${LLD_LINK:-lld-link}"

command -v "$CLANG_CL" >/dev/null 2>&1 || { echo "clang-cl not found: $CLANG_CL" >&2; exit 1; }
command -v "$LLD_LINK" >/dev/null 2>&1 || { echo "lld-link not found: $LLD_LINK" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "python3 not found" >&2; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/runtime/rml/plugins" "$OUT/base"

# Generate the minimal conventional KERNEL32 import library from source.
"$LLD_LINK" /lib /machine:x64 /def:"$ROOT/kernel32_min.def" /out:"$OUT/kernel32_min.lib"

"$CLANG_CL" /nologo /c /O2 /GS- /GR- /EHs-c- /Zl /W4 /WX \
  /Fo"$OUT/EnhancedStorageManagement.obj" "$ROOT/EnhancedStorageManagement.cpp"
# Link under the final DLL filename so the PE export-directory module name is also final.
"$LLD_LINK" /nologo /dll /machine:x64 /entry:DllMain /subsystem:windows \
  /nodefaultlib /dynamicbase /highentropyva /nxcompat /timestamp:0 /opt:ref /opt:icf \
  /out:"$OUT/base/EnhancedStorageManagement.dll" \
  "$OUT/EnhancedStorageManagement.obj" "$OUT/kernel32_min.lib"

python3 "$ROOT/finalize_pe.py" \
  "$OUT/base/EnhancedStorageManagement.dll" \
  "$OUT/runtime/rml/plugins/EnhancedStorageManagement.dll"
cp "$ROOT/EnhancedStorageManagement.ini" "$OUT/runtime/rml/plugins/"
cp "$ROOT/EnhancedStorageManagement.rml.json" "$OUT/runtime/rml/plugins/"
python3 "$ROOT/verify_manifest.py" "$OUT/runtime/rml/plugins/EnhancedStorageManagement.rml.json" "$OUT/runtime/rml/plugins"
python3 "$ROOT/verify_pe.py" "$OUT/runtime/rml/plugins/EnhancedStorageManagement.dll"
sha256sum "$OUT/runtime/rml/plugins/EnhancedStorageManagement.dll"
