# ACP Library

Auto-generated C++ types for the [Agent Client Protocol](https://agentclientprotocol.com/) schema.

## Regenerating

From the project root:

```bash
python scripts/generate_cpp_from_schema.py \
    src/libs/acp/schema/schema.json src/libs/acp/acp.h \
    --namespace Acp \
    --cpp-output src/libs/acp/acp.cpp \
    --export-macro ACPLIB_EXPORT \
    --export-header acp_global.h
```

The generator lives at `scripts/generate_cpp_from_schema.py` and is shared
with the MCP library. Key options:

- `--cpp-output` splits definitions into a separate `.cpp` file (omit for single-header output)
- `--export-macro` adds DLL export/import annotations to declarations in the header
- `--export-header` adds an `#include` for the export macro header
- `--namespace` sets the C++ namespace
- `--no-comments` suppresses doc comments
