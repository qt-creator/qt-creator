# @qtproject/qtprofiler-wasm

The Qt Profiler trace viewer from [Qt Creator](https://www.qt.io/product/development-tools),
built for WebAssembly. It opens QML Profiler traces (`.qtd`, `.qzt`), Common Trace
Format directories and Chrome Trace Format `.json` files.

The build uses a single-threaded Qt for WebAssembly with JavaScript Promise
Integration (JSPI), so it needs no cross-origin isolation, but it does need a
browser that supports JSPI.

## Contents

`dist/` holds the application as Qt for WebAssembly produces it:

| File              | Purpose                                              |
|-------------------|------------------------------------------------------|
| `qtloader.js`     | Qt's loader, which defines `qtLoad()`                |
| `qtprofiler.js`   | The Emscripten module factory                        |
| `qtprofiler.wasm` | The WebAssembly binary                               |
| `qtprofiler.html` | Qt's generic HTML shell; serve `dist/` and open it   |

The package's main module exports their absolute paths:

```js
const qtprofiler = require('@qtproject/qtprofiler-wasm');
qtprofiler.directory;  // .../node_modules/@qtproject/qtprofiler-wasm/dist
qtprofiler.files.wasm; // .../dist/qtprofiler.wasm
```

## Opening a trace

The application takes the same command line as the desktop viewer. Write the
trace into the module's file system before `main()` runs and pass its path as
an argument:

```js
await qtLoad({
    qt: { containerElements: [container] },
    arguments: ['--embedded', '/traces/trace.qtd'],
    preRun: [module => {
        module.FS.mkdir('/traces');
        module.FS.writeFile('/traces/trace.qtd', bytes);
    }],
});
```

For a trace spanning several files, write the whole directory and pass the
directory. `--embedded` hides the controls for opening and closing traces, for
a host that manages them itself. `--rpc` reports progress as JSON-RPC
notifications on standard output, which arrive through `Module.print`.

## License

Commercial Qt licenses, or GPL-3.0-only with the Qt GPL exception 1.0. See
`LICENSES/`.
