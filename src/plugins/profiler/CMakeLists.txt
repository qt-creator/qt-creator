# canSampleOtherProcesses() asks the Security framework what entitlements this
# process was granted, so the sampler can say up front that it cannot attach.
if(APPLE)
  set(PROFILER_APPLE_LIBS "-framework Security")
else()
  set(PROFILER_APPLE_LIBS "")
endif()

# Optional: enables dwarf-mode call-graph unwinding in the Perf Sampler
# backend (perfdwarfunwinder.cpp), via libdw's Dwfl_Thread_Callbacks API.
# libdw/libelf are dual-licensed LGPL-3.0-or-later / GPL-2.0-or-later (see
# /usr/share/doc/libdw-dev/copyright's "Files: lib*" section); the LGPL
# election is what makes linking them into this dual-licensed plugin
# possible, same as perfparser's own (unrelated) use of them under
# CMAKE_MODULE_PATH's Findelfutils.cmake. No qt_attributions.json entry is
# needed: that file covers vendored third-party source shipped in this repo,
# not a dynamically-linked system library found via find_package() -- see
# the ATTRIBUTION_IDS comment below.
# Profiler still builds fine without elfutils: dwarf mode is simply rejected
# with a clear error at PerfSamplerSettings::createSession(), the same
# fail-closed pattern already used for every other unsupported feature in
# perfrecordreader.cpp.
find_package(elfutils)
# PerfDwarfUnwinder handles x86-64 registers only, and the sampler is Linux-only.
set(_profiler_with_libdw OFF)
if(elfutils_FOUND AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
   AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
  set(_profiler_with_libdw ON)
endif()

if(WITH_TESTS)
  set(TEST_SOURCES
    tests/calltreeview_test.cpp tests/calltreeview_test.h
    tests/ctfloader_test.cpp tests/ctfloader_test.h
    tests/ctftimelinemodel_test.cpp tests/ctftimelinemodel_test.h
    tests/debugmessagesmodel_test.cpp tests/debugmessagesmodel_test.h
    tests/fakedebugserver.cpp tests/fakedebugserver.h
    tests/flamegraphmodel_test.cpp tests/flamegraphmodel_test.h
    tests/flamegraphview_test.cpp tests/flamegraphview_test.h
    tests/inputeventsmodel_test.cpp tests/inputeventsmodel_test.h
    tests/localqmlprofilerrunner_test.cpp tests/localqmlprofilerrunner_test.h
    tests/memoryusagemodel_test.cpp tests/memoryusagemodel_test.h
    tests/pixmapcachemodel_test.cpp tests/pixmapcachemodel_test.h
    tests/profilertracedocument_test.cpp tests/profilertracedocument_test.h
    tests/qmlnote_test.cpp tests/qmlnote_test.h
    tests/qmlprofileranimationsmodel_test.cpp tests/qmlprofileranimationsmodel_test.h
    tests/qmlprofilerattachdialog_test.cpp tests/qmlprofilerattachdialog_test.h
    tests/qmlprofilerclientmanager_test.cpp tests/qmlprofilerclientmanager_test.h
    tests/qmlprofilerdetailsrewriter_test.cpp tests/qmlprofilerdetailsrewriter_test.h
    tests/qmlprofilerfindingsmodel_test.cpp tests/qmlprofilerfindingsmodel_test.h
    tests/qmlprofilersampler_test.cpp tests/qmlprofilersampler_test.h
    tests/qmlprofilertool_test.cpp tests/qmlprofilertool_test.h
    tests/qmlprofilertracefile_test.cpp tests/qmlprofilertracefile_test.h
    tests/qmlprofilertraceview_test.cpp tests/qmlprofilertraceview_test.h
    tests/qttracesampler_test.cpp tests/qttracesampler_test.h
    tests/tests.qrc
    tests/perfprofilertracefile_test.cpp tests/perfprofilertracefile_test.h
    tests/perfresourcecounter_test.cpp tests/perfresourcecounter_test.h
    tests/perfnativemixed_test.cpp tests/perfnativemixed_test.h
    tests/perfrecordreader_test.cpp tests/perfrecordreader_test.h
    tests/perfsampler_test.cpp tests/perfsampler_test.h
    tests/dwarflinetable_test.cpp tests/dwarflinetable_test.h
    tests/perfprofilertests.qrc
  )
else()
  set(TEST_SOURCES "")
endif()

set(QMLPROFILER_CPP_SOURCES
  ctfloader.cpp ctfloader.h
  callstacksampler.cpp callstacksampler.h
  calltreemodel.cpp calltreemodel.h
  combinedsampler.cpp combinedsampler.h
  combinedtraceloader.cpp combinedtraceloader.h
  calltreeview.cpp calltreeview.h
  cpuusagemodel.cpp cpuusagemodel.h
  macsampler.cpp macsampler.h
  processpickerdialog.cpp processpickerdialog.h
  qmlprofilersampler.cpp qmlprofilersampler.h
  qttracesampler.cpp qttracesampler.h
  samplemerge.cpp samplemerge.h
  sampler.cpp sampler.h
  samplerrecipe.cpp samplerrecipe.h
  samplertracebackend.cpp samplertracebackend.h
  samplerviewmanager.cpp samplerviewmanager.h
  sampletrace.cpp sampletrace.h
  symbolicator.cpp symbolicator.h
  ctfplainviewmanager.cpp ctfplainviewmanager.h
  ctfstatisticsmodel.cpp ctfstatisticsmodel.h
  ctftracebackend.cpp ctftracebackend.h
  ctfstatisticsview.cpp ctfstatisticsview.h
  ctftimelinemodel.cpp ctftimelinemodel.h
  ctftracemanager.cpp ctftracemanager.h
  ctfvisualizerconstants.h
  ctfvisualizertool.cpp ctfvisualizertool.h
  debugmessagesmodel.cpp debugmessagesmodel.h
  flamegraphmodel.cpp flamegraphmodel.h
  flamegraphview.cpp flamegraphview.h
  inputeventsmodel.cpp inputeventsmodel.h
  memoryusagemodel.cpp memoryusagemodel.h
  pixmapcachemodel.cpp pixmapcachemodel.h
  qmlnote.cpp qmlnote.h
  profilermode.cpp profilermode.h
  profilerplugin.cpp
  profilerrecorder.cpp profilerrecorder.h
  profilersamplerruncontrol.cpp profilersamplerruncontrol.h
  profilerstarteditor.cpp profilerstarteditor.h
  profilertracebackend.cpp profilertracebackend.h
  profilertracedocument.cpp profilertracedocument.h
  profilertraceeditor.cpp profilertraceeditor.h
  profilertr.h
  profiler_global.h
  recordingpage.cpp recordingpage.h
  traceformat.cpp traceformat.h
  welcomepage.cpp welcomepage.h
  qmlprofileranimationsmodel.cpp qmlprofileranimationsmodel.h
  qmlprofilerattachdialog.cpp qmlprofilerattachdialog.h
  qmlprofilerclientmanager.cpp qmlprofilerclientmanager.h
  qmlprofilerconstants.h
  qmlprofilerdashboardstats.cpp qmlprofilerdashboardstats.h
  qmlprofilerdashboardview.cpp qmlprofilerdashboardview.h
  qmlprofilerdetailsrewriter.cpp qmlprofilerdetailsrewriter.h
  qmlprofilereventsview.h
  qmlprofilerfindings.cpp qmlprofilerfindings.h
  qmlprofilerfindingsmodel.cpp qmlprofilerfindingsmodel.h
  qmlprofilerfindingsview.cpp qmlprofilerfindingsview.h
  qmlprofilermodelmanager.cpp qmlprofilermodelmanager.h
  qmlprofilernotesmodel.cpp qmlprofilernotesmodel.h
  qmlprofilerplainviewmanager.cpp qmlprofilerplainviewmanager.h
  qmlprofilerrangemodel.cpp qmlprofilerrangemodel.h
  qmlprofilerrunconfigurationaspect.cpp qmlprofilerrunconfigurationaspect.h
  qmlprofilerruncontrol.cpp qmlprofilerruncontrol.h
  qmlprofilersettings.cpp qmlprofilersettings.h
  qmlprofilerstatemanager.cpp qmlprofilerstatemanager.h
  qmlprofilerstatewidget.cpp qmlprofilerstatewidget.h
  qmlprofilerstatisticsmodel.cpp qmlprofilerstatisticsmodel.h
  qmlprofilerstatisticsview.cpp qmlprofilerstatisticsview.h
  qmlprofilertimelinemodel.cpp qmlprofilertimelinemodel.h
  qmlprofilertracebackend.cpp qmlprofilertracebackend.h
  qmlprofilertool.cpp qmlprofilertool.h
  qmlprofilertracefile.cpp qmlprofilertracefile.h
  qmlprofilertraceview.cpp qmlprofilertraceview.h
  qmlrecordingtimeline.h
  scenegraphtimelinemodel.cpp scenegraphtimelinemodel.h
  quick3dmodel.cpp quick3dmodel.h
  quick3dframeview.cpp quick3dframeview.h
  quick3dframemodel.cpp quick3dframemodel.h
)

# Formerly the separate PerfProfiler plugin, folded in so the perf and QML
# profilers share one tool/perspective/trace clock. Kept in the PerfProfiler
# namespace; see design-docs/native-mixed-profiler-design.md.
set(PERFPROFILER_CPP_SOURCES
  dwarflinetable.cpp dwarflinetable.h
  perfconfigeventsmodel.cpp perfconfigeventsmodel.h
  perfdatareader.cpp perfdatareader.h
  perfevent.h
  perfeventtype.h
  perfloaddialog.cpp perfloaddialog.h
  perfnativemixed.cpp perfnativemixed.h
  perfprofiler.qrc
  profiler.qrc
  perfprofilerconstants.h
  perfprofilerflamegraphmodel.cpp perfprofilerflamegraphmodel.h
  perfprofilertracebackend.cpp perfprofilertracebackend.h
  perfprofilerflamegraphview.cpp perfprofilerflamegraphview.h
  perfprofilerruncontrol.cpp perfprofilerruncontrol.h
  perfprofilerstatisticsmodel.cpp perfprofilerstatisticsmodel.h
  perfprofilerstatisticsview.cpp perfprofilerstatisticsview.h
  perfprofilertool.cpp perfprofilertool.h
  perfprofilertracefile.cpp perfprofilertracefile.h
  perfprofilertracemanager.cpp perfprofilertracemanager.h
  perfprofilertr.h
  perfrecordreader.cpp perfrecordreader.h
  perfresourcecounter.h
  perfrunconfigurationaspect.cpp perfrunconfigurationaspect.h
  perfsampler.cpp perfsampler.h
  perfsettings.cpp perfsettings.h
  perftimelinemodel.cpp perftimelinemodel.h
  perftimelinemodelmanager.cpp perftimelinemodelmanager.h
  perftracepointdialog.cpp perftracepointdialog.h
)

# elfutils/libdw is a system library found via find_package(), not vendored
# source shipped in this repo -- like perfparser's own (much longer-standing)
# dependency on the very same library, it has no qt_attributions.json entry:
# that file is for bundled third-party source with a LicenseFile this repo
# actually ships, which doesn't apply to a dynamically-linked system library.
if(_profiler_with_libdw)
  list(APPEND PERFPROFILER_CPP_SOURCES perfdwarfunwinder.cpp perfdwarfunwinder.h)
endif()

# QtSupport is only used for the live-profiling launch integration (selecting a Qt
# kit/version to run and profile an app), which is unavailable on WebAssembly. Drop it
# there so the offline trace viewer does not drag in QtSupport -> ProParser -> qmake.
set(_profiler_qtsupport_dep QtSupport)
if (EMSCRIPTEN)
  set(_profiler_qtsupport_dep "")
endif()

add_qtc_plugin(Profiler
  CONDITION TARGET Tracing
  DEPENDS Qt::Network QmlDebug QmlJS Tracing CommonTraceFormat QtTaskTree ${PROFILER_APPLE_LIBS}
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  PLUGIN_DEPENDS Core ProjectExplorer ${_profiler_qtsupport_dep} TextEditor
  SOURCES
    ${TEST_SOURCES}
    ${QMLPROFILER_CPP_SOURCES}
    ${PERFPROFILER_CPP_SOURCES}
    ../../libs/3rdparty/json/json.hpp
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      json-nlohmann
)

extend_qtc_plugin(Profiler
  SOURCES mcpsupport.cpp mcpsupport.h
  DEPENDS McpServerLib
)

# The Windows sampler backend is only compiled on Windows (its sources use
# Windows APIs without Q_OS_WIN guards) and on every Windows toolchain, so the
# libraries it needs are linked for all of them, not just MSVC.
extend_qtc_plugin(Profiler CONDITION WIN32
  DEPENDS dbghelp psapi
  SOURCES
    etwlauncher_win.cpp etwlauncher_win.h
    winsampler.cpp winsampler.h
    winsymbolicator.cpp winsymbolicator.h
)

if(TARGET Profiler AND _profiler_with_libdw)
  target_link_libraries(Profiler PRIVATE elfutils::dw elfutils::elf)
  target_compile_definitions(Profiler PRIVATE WITH_LIBDW)
endif()

# PerfDataReader (perfdatareader.cpp, backing the CPU Usage analyzer) shells
# out to perfparser at runtime; building Profiler (and anything that depends
# on it, e.g. qtprofiler) should build it too. (The Perf Sampler backend in
# perfsampler.cpp does not use perfparser -- it decodes "perf record"'s output
# itself, see perfrecordreader.cpp.) src/plugins is processed before
# src/tools, so perfparser does not exist yet at this point; defer the check
# to the end of the top-level directory, by which time src/tools/perfparser
# has run. perfparser only exists when elfutils was found (see
# src/tools/perfparser/CMakeLists.txt), so this is a no-op, not an error, when
# it is absent.
function(qtc_profiler_add_perfparser_dependency)
  if (TARGET perfparser)
    add_dependencies(Profiler perfparser)
  endif()
endfunction()
if (TARGET Profiler)
  cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL qtc_profiler_add_perfparser_dependency)
endif()
