# How to generate the protocol files

The types come from the meta model published with the language server protocol
specification, `metaModel.json`, in two stages: the meta model is converted to
a JSON schema, which is then turned into C++.

```bash
# Run from the root of the repository
python3 scripts/lsp_metamodel_to_json_schema.py \
    src/libs/languageserverprotocol/metaModel.json \
    src/libs/languageserverprotocol/lsp.schema.json \
    --messages-output src/libs/languageserverprotocol/lspmessages.h

python3 scripts/generate_cpp_from_schema.py \
    src/libs/languageserverprotocol/lsp.schema.json \
    src/libs/languageserverprotocol/lsptypes.h \
    --namespace LanguageServerProtocol \
    --cpp-output src/libs/languageserverprotocol/lsptypes.cpp \
    --export-macro LANGUAGESERVERPROTOCOL_EXPORT \
    --export-header languageserverprotocol_global.h \
    --no-cxx20
```

`--no-cxx20` makes `fromJson` propagate errors with explicit early returns
instead of `co_await`. The coroutine support in `utils/co_result.h` relies on
the conversion of the coroutine's return object being delayed until the
coroutine body has run, which Clang only does since version 17.

To move to a newer specification, replace `metaModel.json` with the one
published for it and run both commands again. Every generated file records the
command that produced it, so a file can also be re-derived from its own banner.
