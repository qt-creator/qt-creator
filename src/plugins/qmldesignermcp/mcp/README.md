# QML MCP server runtime

`qml_mcp_server.py` is shipped as a standalone bundle built by PyInstaller from
`qml_mcp_server.spec`. PyInstaller bundles the interpreter it runs under, so
`pyproject.toml` and `uv.lock` here decide what the product ships: the CPython
patch version, every Python distribution in the payload, and — because OpenSSL
is part of a python-build-standalone interpreter rather than a separate
dependency — the OpenSSL version too.

That makes these two files security-relevant, not merely a developer
convenience.

## Why the settings look like this

- `requires-python` is an exact patch pin. It selects the interpreter, and with
  it the OpenSSL build. Nothing else in the tree records that version.
- `python-preference = "only-managed"` and the `--managed-python` flag in
  `CMakeLists.txt` keep uv from silently satisfying the pin with a system
  interpreter. Without them the payload depends on whatever the build host
  happens to have installed, which is the failure this project exists to
  remove. The command line flag is what wins over `UV_PYTHON_PREFERENCE` in
  the environment.
- The uv version bounds which interpreter is reachable at all. uv freezes its
  catalogue of downloadable Python builds per release, and both `--locked` and
  `--managed-python` need a uv recent enough to have them. Neither is enforced
  from here, so an old uv fails the sync instead of being told what is wrong.
- The build runs `uv sync --locked`, which rejects a lock file that no longer
  matches `pyproject.toml`. `--frozen` would install the stale pin instead.

## Checking which runtime a build used

Nothing enforces the pinned path. When uv is missing the build warns and
continues with the ambient interpreter, so a green build is not evidence that
the pin took effect. The configure log records which interpreter was used:

    -- Using '<build>/mcp-venv/bin/python' to generate the QML MCP server.

If that line points elsewhere, or a warning about uv precedes it, the artifact
carries an unpinned runtime and payload and must not be shipped.

## Updating the runtime or a dependency

1. Change the pin in `pyproject.toml`. Never edit `uv.lock` by hand.
2. Regenerate the lock with `uv lock` and read its diff: direct pins, registry
   origin, hash presence, added and removed package nodes, platform markers.
3. Rebuild the bundle and check the payload, not just the lock. The deny list in
   `qml_mcp_server.spec` fails the build on a forbidden package, but it is a
   product policy rather than an inventory of what ships.
4. Repeat on macOS, Windows and Linux. The lock file is universal; the payload is
   not, and OpenSSL reaches the artifact differently per platform — as its own
   DLLs on Windows, linked into libpython elsewhere. Whether `_ssl` is builtin
   can differ too, which changes what the dylib collector does on macOS.

## Moving OpenSSL

OpenSSL is not a dependency here. It is compiled into the interpreter that
python-build-standalone publishes, so the only way to move it is to move
`requires-python` — and nothing in the tree records which OpenSSL a given pin
carries. It has to be measured.

- Find which CPython patch release carries the version you want. The
  python-build-standalone release notes list the CPython and OpenSSL bumps side
  by side, but they are independent: one CPython version can be rebuilt in
  several releases against different OpenSSL, and uv resolves a pin to the newest
  release containing it. The pin names a version, not an artifact.
- Check it is reachable with `uv python list --all-versions`. If it is missing,
  the build hosts need a newer uv first.
- Read what actually arrived:
  `python -c "import ssl; print(ssl.OPENSSL_VERSION)"`. The SBOM entry carries
  the same value, so every build records the version it shipped.
- A newer OpenSSL is not automatically an unaffected one. Check the advisories
  for the branch that arrives, not only for the one being left behind.

## What is deliberately included

The dependency closure carries an HTTP stack (`starlette`, `uvicorn`,
`sse_starlette`, `httpx`) and `cryptography` by way of `pyjwt[crypto]`. The
stdio entry point reaches none of it, but `mcp` declares all of it
unconditionally rather than behind extras, so it cannot be dropped by
declaration. It is kept so an HTTP transport stays possible; the deny list, not
the lock file, is the lever for removing it from the artifact.
