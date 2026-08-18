# ACP Library

Auto-generated C++ types for the [Agent Client Protocol](https://agentclientprotocol.com/) schema.

## Regenerating

From the project root:

```bash
# Protocol v1 (namespace Acp)
python scripts/generate_cpp_from_schema.py \
    src/libs/acp/schema/schema.json src/libs/acp/acp.h \
    --namespace Acp \
    --cpp-output src/libs/acp/acp.cpp \
    --export-macro ACPLIB_EXPORT \
    --export-header acp_global.h

# Protocol v2 (namespace Acp::V2)
python scripts/generate_cpp_from_schema.py \
    src/libs/acp/schema/schema-v2.json src/libs/acp/acpv2.h \
    --namespace Acp::V2 \
    --cpp-output src/libs/acp/acpv2.cpp \
    --export-macro ACPLIB_EXPORT \
    --export-header acp_global.h \
    --three-state
```

`schema-v2.json` is the stable v2 baseline from the upstream
agent-client-protocol repository (`schema/v2/schema.json`);
`meta-v2.json` is the corresponding method-name table (documentation
only, not consumed by the generator). v2 requires `--three-state`
because its update payloads distinguish omitted fields (leave
unchanged) from explicit null (clear).

The generator lives at `scripts/generate_cpp_from_schema.py` and is shared
with the MCP library. Key options:

- `--cpp-output` splits definitions into a separate `.cpp` file (omit for single-header output)
- `--export-macro` adds DLL export/import annotations to declarations in the header
- `--export-header` adds an `#include` for the export macro header
- `--namespace` sets the C++ namespace
- `--no-comments` suppresses doc comments
- `--three-state` models optional nullable fields as `Patch<T>` (absent/null/value)
