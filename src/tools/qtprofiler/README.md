Building qtprofiler for WebAssembly
===================================

qtprofiler also builds as a WebAssembly application, so a trace can be viewed
in a browser, or embedded in another application's web view (see --embedded).

The build described here is **single-threaded**. Nothing in it needs
SharedArrayBuffer, and so nothing needs the page to be cross-origin
isolated: it is served, and embedded, like any other page. That matters for
embedding, since a host such as a VS Code web view is not cross-origin
isolated and cannot easily be made so.

It also builds against a multithreaded Qt for WebAssembly, where none of the
machinery below applies: the trace is parsed on a thread of its own,
asyncYield() compiles to nothing, and neither JSPI nor Asyncify is linked in.
Such a build does need the page to be cross-origin isolated, because its
threads are Web Workers sharing memory.


Why Qt has to be built for this
-------------------------------

Without thread support QThread::start() starts nothing, so the thread pool
behind Utils::asyncRun() never runs its callable and a trace would never finish
loading. There asyncRun() runs the callable on the main thread instead, and the
trace loaders call Utils::asyncYield() as they go, which suspends back to the
browser and resumes once it has had its turn. Suspending needs one of:

- **JSPI**, the browser's own stack switching. The wasm module is left alone,
  so this costs next to nothing. Qt must be configured with it, and it requires
  native wasm exception handling.
- **Asyncify**, a binaryen pass over the whole module. It works with the
  prebuilt Qt packages and needs no Qt build, but it instruments ~42000
  functions and the module grows by about 60% (34.7 MiB to 55.6 MiB).

The prebuilt Qt for WebAssembly packages ship with `wasm-jspi` *and*
`wasm-exceptions` disabled, which is why JSPI needs a Qt of your own. With such
a Qt, `src/tools/qtprofiler/CMakeLists.txt` leaves Asyncify out and Qt supplies
the JSPI link options itself.


Building Qt with JSPI
---------------------

You need the Qt sources, a host Qt of the same version whose tools run during
the cross build, and an installed emsdk. Qt 6.11.2 does not build without style
sheet support or without the CBOR stream writer as it is; apply the patches in
`qt-patches/qtbase` to its qtbase first (see below):

    cd <qt-src>/qtbase && git apply <this-dir>/qt-patches/qtbase/*.patch
    cd <somewhere>
    <qt-src>/configure -platform wasm-emscripten \
      -prefix <install-dir> \
      -qt-host-path <host-qt> \
      -feature-wasm-exceptions \
      -feature-wasm-jspi \
      -release -optimize-size \
      -nomake examples -nomake tests \
      -submodules qtbase,qtsvg \
      -skip qtimageformats \
      -no-feature-jpeg -no-feature-gif -no-feature-ico \
      -no-feature-imageformat_jpeg \
      -no-feature-sql -no-feature-testlib \
      -no-feature-mdiarea -no-feature-calendarwidget -no-feature-fontdialog \
      -no-feature-whatsthis -no-feature-sessionmanager \
      -no-feature-printpreviewwidget \
      -no-feature-hijricalendar -no-feature-jalalicalendar \
      -no-feature-islamiccivilcalendar \
      -no-feature-scroller -no-feature-lcdnumber -no-feature-errormessage \
      -no-feature-undogroup -no-feature-socks5 \
      -no-feature-raster-fp -no-feature-sha3-fast -no-feature-mimetype-database \
      -no-feature-imageformat_bmp -no-feature-imageformat_ppm \
      -no-feature-imageformat_xbm \
      -no-feature-cborstreamwriter -no-feature-datetimeparser \
      -no-feature-sizegrip -no-feature-tabletevent \
      -no-feature-dial -no-feature-toolbox -no-feature-rubberband \
      -no-feature-graphicsview \
      -no-feature-style-stylesheet
    cmake --build . --parallel
    cmake --install .

Pass **both** wasm features: Qt only offers JSPI when wasm-exceptions is on,
because JSPI cannot suspend across the JavaScript frames that the emulated
exception and setjmp/longjmp trampolines put on the stack.

Do not add `-feature-thread`. The configure summary should end up with

    WebAssembly Exceptions ... yes
    WebAssembly JSPI ........ yes
    Thread support .......... no

The rest of the line is about size:

- `-optimize-size` compiles Qt with `-Oz`, the largest single cut there is:
  3.8 MB off the module. It costs speed where Qt does the work: an XML .qtd
  trace, which QXmlStreamReader reads, took about twice as long to parse in a
  single measurement, while a Common Trace Format trace took the same.
- `-submodules qtbase,qtsvg` is everything qtprofiler links. Qt Declarative,
  Qt Shader Tools, Qt WebSockets, Qt HTTP Server and Qt Canvas Painter are not
  linked at all; without them the Qt build takes 1,136 steps instead of 3,136.
  Qt Creator's configure requires Qt Qml and Qt Sql on every platform but
  WebAssembly.
- Leaving `qtimageformats` out of `-submodules` does *not* exclude it: it is
  still built and still linked. `-skip qtimageformats` is the flag that does.
  Nothing here decodes anything but PNG and SVG, which is also why the other
  image format features go.
- The features in the last blocks, up to `-no-feature-imageformat_xbm`, are
  not used by anything qtprofiler links, and cost 546 KB between them.
  `raster-fp` is floating point image formats, `sha3-fast` an unrolled SHA-3
  where a compact one does, and `mimetype-database` Qt's copy of the MIME
  database, which Utils does not read: it has one of its own.
  `-no-feature-testlib` also drops the `-test` options ExtensionSystem
  otherwise adds whenever Qt Test exists. `-no-feature-style-stylesheet` needs
  more than a flag; see below.
- `cborstreamwriter`, `datetimeparser`, `sizegrip`, `tabletevent`, `dial`,
  `toolbox` and `rubberband` are unused as well, and take another 66 KB off.
  `cborstreamwriter` needs a fix to Qt; see below.
- `-no-feature-graphicsview` takes 254 KB off. Nothing qtprofiler links uses
  a graphics scene, but Qt Widgets compiles its support for one into
  QWidget, QApplication and others whenever the feature is on. It turns
  `graphicseffect` off with it, and so the opacity effects Qt Creator fades
  progress bars and indicators with: built without them, Utils and Core
  show and hide those at once, when a fade would have started.

Some features that look just as unused are not: `wizard`, `colordialog`,
`undocommand` and `undostack`, and `textodfwriter` are used by Utils and Core,
and the build fails without them. So is `movie`: Utils' MarkdownBrowser plays
animated images. `effects` cannot go either: Utils' tool tips call Qt's
qFadeEffect(), and `effects` is a private feature, which Utils could only test
through Qt's private Widgets headers. `picture` takes `printer` with it, which
Core uses. And `raster-64bit` builds, but changes what is drawn: a glyph that
only partly fits into one of the flame graph's narrow boxes is no longer drawn
at all. Check with a grep over the sources of what is
actually built before adding one, and read what configure turned off with it:
diffing the generated `*-config.h` headers against a previous build shows every
feature that changed.

`-no-feature-style-stylesheet` takes another 132 KB off, and changes nothing
on screen: the dashboard and the flame graph come out pixel for pixel the same.
Qt Creator only sets a style sheet in widgets qtprofiler never shows, behind
`QT_CONFIG(style_stylesheet)`. Qt itself needs two fixes for it, in
`qt-patches/qtbase`:

- `0001` guards the definition of `QTipLabel::styleSheetParentDestroyed()`.
  It is upstream Qt's own commit 5ffacee8b9c0, which is on the 6.11 branch
  and so will be in 6.11.3.
- `0002` guards `qcommandlinkbutton.cpp`'s include of `qstylesheetstyle_p.h`
  and its cast to `QStyleSheetStyle`, both unconditional since commit
  839d569dc27c. It is in review upstream as
  https://codereview.qt-project.org/c/qt/qtbase/+/774919, for dev and, by
  its Pick-to line, 6.12, 6.11 and 6.10.

`-no-feature-cborstreamwriter` needs a third:

- `0003` guards `removed_api.cpp`'s include of `qcborstreamwriter.h`,
  unconditional since commit c17a934197a0, so that Qt Core builds without the
  feature at all. It is in review upstream as
  https://codereview.qt-project.org/c/qt/qtbase/+/775735, for dev and, by its
  Pick-to line, 6.12, 6.11 and 6.10.

The Dockerfile applies all three, and skips one the sources already contain.

The MIME database Utils starts from is Apache Tika's, 1665 types in 317 KB of
XML. qtprofiler looks up no MIME type of its own, so the WebAssembly build
ships `src/libs/utils/mimetypes2/basic-mimetypes.xml` in its place: the types
MimeDatabase falls back to and those the Profiler plugin's own types derive
from.

`-no-opengl` would take about 154 KB by the link map and cannot be had: the
wasm platform plugin only builds its raster backing store, which every widget
window paints through, when `opengl` is on.

`-no-feature-accessibility` takes another 143 KB off (56 KB gzipped) and
builds: Utils guards what it copied from Qt for PlainTextEdit's accessibility,
as Qt does. Nothing changes on screen, but a screen reader is left with nothing
to read, so it stays on.

Do **not** add `-no-feature-imageformat_xpm`: `draganddrop` is
`CONDITION QT_FEATURE_imageformat_xpm`, so turning XPM off silently turns drag
and drop off with it, and Qt Creator does not compile without it.


Building qtprofiler
-------------------

`scripts/build-wasm-profiler.py` drives the `qtprofiler-wasm` CMake preset and
takes everything it needs on the command line:

    scripts/build-wasm-profiler.py \
        --qt-wasm <install-dir> \
        --qt-host <host-qt> \
        --emsdk <emsdk> \
        --output artifacts

Point `--qt-wasm` at the Qt built above for a JSPI build, or at a prebuilt
`wasm_singlethread` package for an Asyncify one. The result is a directory
holding `qtprofiler.html`, `qtprofiler.js`, `qtprofiler.wasm`, `qtloader.js`
and `qtlogo.svg`; serve it over HTTP and open the .html.

The preset builds `Release`, at `-O3`. Building the application at `-Oz`
instead (`MinSizeRel` with `CMAKE_CXX_FLAGS_MINSIZEREL` set to `-Oz -DNDEBUG`,
which also makes the final wasm-opt pass optimize for size) takes another
3.2 MB off, but costs parse speed in Qt Creator's own loaders: in a single
measurement a 57 MB combined trace parsed in 1.3 s instead of 1.1 s.

Qt's platform plugin bundles DejaVu Sans and DejaVu Sans Mono in full, about
1 MB of the module. qtprofiler links a subset of them in their place, keeping
Latin, Greek, Cyrillic and the punctuation and symbol blocks, for 463 KB less.
That needs `pyftsubset` (from fonttools) and the fonts themselves, which only
Qt's sources have: the script finds them through the source tree the Qt
install records, so it happens for a Qt built as above and not for a prebuilt
one. Without either, qtprofiler keeps Qt's fonts. The fonts have to stay in
some form: a VS Code web view does not grant `local-fonts`, so they are the
only fonts the application has (see below).


Building in a container
-----------------------

`Dockerfile` next to this file builds everything above into an image: emsdk
4.0.7, a host Qt, a Qt for WebAssembly configured as described above, and
fonttools for the font subset. The image holds no Qt Creator sources; it
builds whichever checkout it is given:

    docker build -t qtprofiler-wasm src/tools/qtprofiler
    docker run --rm -v "$PWD:/src" qtprofiler-wasm

The second command, run in a checkout, builds it with
`scripts/build-wasm-profiler.py` in `builds/wasm-docker` and leaves the files
to serve in `builds/wasm-docker/dist`. Arguments after the image name are
passed on to the script, for example `--jobs 4`. On Linux, add
`--user "$(id -u):$(id -g)"` so that the build is not owned by root.

With eight cores the image takes about seven minutes to build and is 2.5 GB;
qtprofiler then builds in about four. Its module matches a local build of the
same configuration but for the source paths compiled into it, which are
shorter under `/src`: 27 KB of the 15.9 MB.

The image builds qtbase and qtsvg one at a time rather than through the
top-level configure, so only their sources are downloaded, and `-submodules`
and `-skip` have no counterpart there. For another Qt version pass
`--build-arg QT_VERSION=...` together with the checksums of its two tarballs,
`QTBASE_SHA256` and `QTSVG_SHA256`, and check that `EMSDK_VERSION` is still
the one Qt recommends.


Handing a trace to it
---------------------

The application takes its trace as a positional argument, so an embedding page
can write the bytes into the module's in-memory filesystem in `preRun` and pass
the path in `arguments`. A trace that is a directory (a Common Trace Format
trace, a recorded sampler trace, a combined bundle) is handed over as the
directory, with every file written under it.

`--rpc` reports progress as JSON-RPC 2.0 notifications on stdout, which
emscripten delivers to the page's `print` callback; `--rpc-schema` prints the
schema. `--embedded` hides everything that would open, switch or close a trace,
for a host that decides which trace is shown.


Size
----

Measured on a 6.11.2 JSPI build, .wasm bytes, against the same sources, each
row against the one above it. The gzipped column is `gzip -9`, what a server
ships; the default level gives numbers about half a percent higher and will not
reproduce these:

| change                                  |        raw |     gzipped |
|-----------------------------------------|-----------:|------------:|
| baseline                                | 38,044,399 |  13,147,981 |
| leaving out the GPU track backend       |   -714,910 |    -478,236 |
| the QML/Quick `-no-feature` list        |    -95,563 |     -32,392 |
| the image format features               |   -512,164 |    -119,969 |
| `-skip qtimageformats`                  |   -721,955 |    -246,353 |
| the in-memory settings database (SQLite)| -1,542,839 |    -638,742 |
| leaving out the Qt Creator integration  | -6,067,881 |  -1,720,877 |
| leaving out the JavaScript engine       | -3,222,330 |  -1,001,852 |
| leaving out the QML and C++ parsers     | -1,707,418 |    -451,287 |
| leaving out the text editor             | -2,475,449 |    -877,819 |
| Qt `-optimize-size`                     | -3,845,311 |    -993,491 |
| the font subset                         |   -463,321 |    -208,211 |
| the remaining `-no-feature` list        |   -351,309 |    -149,269 |
| the style sheet engine, with Qt patched |   -131,565 |     -53,562 |
| the MIME database cut to basic types    |    -44,591 |     -43,842 |
| five more `-no-feature`s                |   -194,624 |     -70,912 |
| seven more `-no-feature`s               |    -66,174 |     -27,890 |
| no graphics view, and so no fades       |   -254,226 |    -109,262 |
| **all of them**                         | 15,605,167 |   5,923,287 |

That is 59.0% off the raw module and 54.9% off what a server ships. The last
six rows were measured in the container (see "Building in a container"), where
the module comes out 27 KB smaller than a local build for its shorter source
paths; each of the five compares two container builds. The
QML/Quick list no longer appears in the configure line above: nothing
qtprofiler links uses Qt Declarative any more, so it is not built at all.

The builds from the style sheet row on show the combined trace's dashboard
with the same numbers as the `-O2` build does. The last one renders the
dashboards of a combined, a QML, a Chrome and a sampler trace, and the QML
trace's timeline and flame graph, pixel for pixel as the build before the last
two rows does, and without an error.

Debug info is already out of a `Release` build: it is compiled `-O3 -DNDEBUG`
with no `-g`, and the .wasm carries no name section.

Two things that sound like size wins and are not, both measured before
`-optimize-size`:

- **`-ltcg`** (link time code generation) makes the module about 10% *bigger*
  (+3.8 MB). At `-O3` it inlines across translation units far more than it
  folds away.
- **`-feature-wasm-simd128`** costs about 1.2 MB for the vectorised paths. It
  is a speed feature; enable it for that, not for size.

And one that is worse than either: building the application with
`CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`. At `-Oz` it makes the module 41 KB
bigger, and the link reports function signature mismatches between the LTO
and the ordinary objects (`Core::IDocument::saveImpl`,
`QAbstractItemModel::roleNames` and others), each a call that traps when it is
made. An earlier configuration aborted with `RuntimeError: null function` as
soon as a trace was loaded; this one loads a combined trace, but only because
none of the mismatched functions is called while a trace is viewed.

### Finding the next one

Guessing at features is a poor way to do this; ask the linker instead. Build
with a map file and add up what each archive actually contributed:

    cmake -B <build> -DCMAKE_EXE_LINKER_FLAGS="-Wl,-Map=/tmp/qtprofiler.map"

Each line is `addr off size <archive>.a(<member>.o):(<symbol>)`, so summing the
hex size column per archive says where the bytes went. Note that an archive's
size is an upper bound on what removing it saves: the linker only ever pulls
part of each, so the 5.2 MB of Qt Qml and its engine came out as 3.2 MB of
module.

The fonts show how far a static link lets the application reach into Qt. Qt's
platform plugin loads `:/fonts/DejaVuSans.ttf`, and the bytes sit in the
archive member `qrc_wasmfonts.cpp.o`, which the linker pulls in only because
an always-linked initializer calls `qInitResources_wasmfonts()`. Define that
function in the application, with a resource of the same name and paths, and
the member is never pulled: the application's subset is what gets loaded, and
Qt is not patched.

The fonts cannot simply go. Without them the application has no font at all
in a VS Code web view: the browser's own fonts reach a page through
`window.queryLocalFonts()`, and in a web view that call raises a
`SecurityError` because `local-fonts` is absent from the frame's permissions
policy -- 26 other features are allowed there, so the omission is deliberate.
Reading the same call in the developer tools succeeds and suggests otherwise,
because the tools evaluate it against the top frame while the content runs one
frame down.

What the largest entries are now, and why each is still there:

| entry                          |  linked | why |
|--------------------------------|--------:|-----|
| Qt Widgets, Gui, Core          | 9.1 MB  | the floor for a widget application |
| Qt Creator's Core, Utils, Profiler | 6.7 MB | what the application is; 4.7 MB at `-Oz` |
| HarfBuzz, FreeType             | 1.1 MB  | shaping and rasterizing text; with no local fonts there is no other way to draw any |
| `qrc_qstyle.cpp.o`             | 351 KB  | Qt Widgets' standard pixmaps, 203 PNGs; about half are drive, media and dialog-button icons the Fusion style never shows |
| `libQt6Network.a`              | 326 KB  | wanted by Utils, Core and QmlDebug |

The style pixmaps could be replaced the way the fonts are, keeping only the
ones in use. Unlike a missing glyph, a missing pixmap is drawn as nothing and
reported nowhere, so a list that goes stale with a Qt update fails silently.

`libProjectExplorer.a` (6.1 MB), `libQt6Qml.a` (4.2 MB), `libqsqlite.a`
(1.7 MB), `libquickjsng.a` (983 KB), the image format plugins and the full
fonts used to be the largest entries here, and all of them are gone.
