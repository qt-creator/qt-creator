# canSampleOtherProcesses() asks the Security framework what entitlements this
# process was granted, so the sampler can say up front that it cannot attach.
if(APPLE)
  set(PROFILER_APPLE_LIBS "-framework Security")
else()
  set(PROFILER_APPLE_LIBS "")
endif()

# Optional: enables unwinding dwarf-mode call graphs of perf recordings
# (perfdwarfunwinder.cpp), via libdw's Dwfl_Thread_Callbacks API, and
# resolving the functions inlined at an address (perfsymbolizer.cpp).
# libdw/libelf are dual-licensed LGPL-3.0-or-later / GPL-2.0-or-later (see
# /usr/share/doc/libdw-dev/copyright's "Files: lib*" section); the LGPL
# election is what makes linking them into this dual-licensed plugin
# possible. No qt_attributions.json entry is needed: that file covers
# vendored third-party source shipped in this repo, not a dynamically-linked
# system library found via find_package() -- see the ATTRIBUTION_IDS comment
# below.
# Profiler still builds fine without elfutils: dwarf mode is simply rejected
# with a clear error at PerfSamplerSettings::createSession(), the same
# fail-closed pattern already used for every other unsupported feature in
# perfrecordreader.cpp.
find_package(elfutils)
# Linux only: elsewhere, the plugin would need libdw deployed where it finds it.
set(_profiler_with_libdw OFF)
if(elfutils_FOUND AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
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
    tests/perfdataparser_test.cpp tests/perfdataparser_test.h
    tests/perftraceconverter_test.cpp tests/perftraceconverter_test.h
    tests/perfrecordreader_test.cpp tests/perfrecordreader_test.h
    tests/perfsamplersettings_test.cpp tests/perfsamplersettings_test.h
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
  profilerrecorder.cpp profilerrecorder.h
  profilertracebackend.cpp profilertracebackend.h
  profilertracedocument.cpp profilertracedocument.h
  profilertraceeditor.cpp profilertraceeditor.h
  profilertr.h
  profiler_global.h
  recordingpage.cpp recordingpage.h
  traceformat.cpp traceformat.h
  welcomepage.cpp welcomepage.h
  qmlprofileranimationsmodel.cpp qmlprofileranimationsmodel.h
  qmlprofilerclientmanager.cpp qmlprofilerclientmanager.h
  qmlprofilerconstants.h
  qmlprofilerdashboardstats.cpp qmlprofilerdashboardstats.h
  qmlprofilerdashboardview.cpp qmlprofilerdashboardview.h
  qmlprofilerdetailsrewriter.h
  qmlprofilereventsview.h
  qmlprofilerfindings.cpp qmlprofilerfindings.h
  qmlprofilerfindingsmodel.cpp qmlprofilerfindingsmodel.h
  qmlprofilerfindingsview.cpp qmlprofilerfindingsview.h
  qmlprofilermodelmanager.cpp qmlprofilermodelmanager.h
  qmlprofilernotesmodel.cpp qmlprofilernotesmodel.h
  qmlprofilerplainviewmanager.cpp qmlprofilerplainviewmanager.h
  qmlprofilerrangemodel.cpp qmlprofilerrangemodel.h
  qmlprofilersettings.cpp qmlprofilersettings.h
  qmlprofilerstatemanager.cpp qmlprofilerstatemanager.h
  qmlprofilerstatewidget.cpp qmlprofilerstatewidget.h
  qmlprofilerstatisticsmodel.cpp qmlprofilerstatisticsmodel.h
  qmlprofilerstatisticsview.cpp qmlprofilerstatisticsview.h
  qmlprofilertimelinemodel.cpp qmlprofilertimelinemodel.h
  qmlprofilertracebackend.cpp qmlprofilertracebackend.h
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
  perfnativemixed.cpp perfnativemixed.h
  perfprofiler.qrc
  profiler.qrc
  perfprofilerconstants.h
  perfprofilerflamegraphmodel.cpp perfprofilerflamegraphmodel.h
  perfprofilertracebackend.cpp perfprofilertracebackend.h
  perfprofilerflamegraphview.cpp perfprofilerflamegraphview.h
  perfprofilerstatisticsmodel.cpp perfprofilerstatisticsmodel.h
  perfprofilerstatisticsview.cpp perfprofilerstatisticsview.h
  perfprofilertracefile.cpp perfprofilertracefile.h
  perfprofilertracemanager.cpp perfprofilertracemanager.h
  perfprofilertr.h
  perfconversion.cpp perfconversion.h
  perfdataparser.cpp perfdataparser.h
  perfrecordreader.cpp perfrecordreader.h
  perfregisters.h
  perfsymbolizer.cpp perfsymbolizer.h
  perftraceconverter.cpp perftraceconverter.h
  perftracepointformats.cpp perftracepointformats.h
  perfresourcecounter.h
  perfsampler.cpp perfsampler.h
  perfsettings.cpp perfsettings.h
  perftimelinemodel.cpp perftimelinemodel.h
  perftimelinemodelmanager.cpp perftimelinemodelmanager.h
)

# elfutils/libdw is a system library found via find_package(), not vendored
# source shipped in this repo, so it has no qt_attributions.json entry: that
# file is for bundled third-party source with a LicenseFile this repo
# actually ships, which doesn't apply to a dynamically-linked system library.
if(_profiler_with_libdw)
  list(APPEND PERFPROFILER_CPP_SOURCES perfdwarfunwinder.cpp perfdwarfunwinder.h)
endif()

# QtSupport is only used for the live-profiling launch integration (selecting a Qt
# kit/version to run and profile an app), which is unavailable on WebAssembly. Drop it
# there so the offline trace viewer does not drag in QtSupport -> ProParser -> qmake.
set(_profiler_qtsupport_dep QtSupport)
set(_profiler_projectexplorer_dep ProjectExplorer)
# Only the details rewriter parses QML, and it cannot resolve a source file on
# WebAssembly (see qmlprofilerdetailsrewriter_wasm.cpp), so the parser - and
# the C++ one behind it - are left out there.
set(_profiler_qmljs_dep QmlJS)
# TextEditor is only used to put text marks in Qt Creator's editors, which the
# standalone viewer does not have.
set(_profiler_texteditor_dep TextEditor)
if (QTPROFILER_WASM)
  set(_profiler_qtsupport_dep "")
  set(_profiler_projectexplorer_dep "")
  set(_profiler_qmljs_dep "")
  set(_profiler_texteditor_dep "")
endif()

# Everything that integrates the profiler into Qt Creator: the mode, the plugin
# entry point, the run controls and run-configuration aspects, and the tool
# classes owning a Qt Creator perspective. The WebAssembly build is the
# standalone viewer, which loads no plugins and cannot run a target, so none of
# it is reachable there - and leaving it out is what lets that build do without
# ProjectExplorer.
set(PROFILER_CREATOR_SOURCES
  perfloaddialog.cpp perfloaddialog.h
  perfprofilerruncontrol.cpp perfprofilerruncontrol.h
  perfprofilertool.cpp perfprofilertool.h
  perfrunconfigurationaspect.cpp perfrunconfigurationaspect.h
  perftracepointdialog.cpp perftracepointdialog.h
  profilermode.cpp profilermode.h
  profilerplugin.cpp
  profilersamplerruncontrol.cpp profilersamplerruncontrol.h
  profilerstarteditor.cpp profilerstarteditor.h
  qmlprofilerattachdialog.cpp qmlprofilerattachdialog.h
  qmlprofilerrunconfigurationaspect.cpp qmlprofilerrunconfigurationaspect.h
  qmlprofilerruncontrol.cpp qmlprofilerruncontrol.h
  qmlprofilertool.cpp qmlprofilertool.h
)

add_qtc_plugin(Profiler
  CONDITION TARGET Tracing
  DEPENDS Qt::Network QmlDebug ${_profiler_qmljs_dep} Tracing CommonTraceFormat QtTaskTree ${PROFILER_APPLE_LIBS}
  # The CTF loader includes <json/json.hpp> directly; own that include path
  # rather than inheriting it from whichever plugin happens to be built too.
  INCLUDES ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/libs/3rdparty
  PLUGIN_DEPENDS Core ${_profiler_projectexplorer_dep} ${_profiler_qtsupport_dep} ${_profiler_texteditor_dep}
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
  CONDITION NOT QTPROFILER_WASM
  SOURCES ${PROFILER_CREATOR_SOURCES}
)

extend_qtc_plugin(Profiler
  CONDITION QTPROFILER_WASM
  SOURCES qmlprofilerdetailsrewriter_wasm.cpp
)

extend_qtc_plugin(Profiler
  CONDITION NOT QTPROFILER_WASM
  SOURCES qmlprofilerdetailsrewriter.cpp
)

extend_qtc_plugin(Profiler
  CONDITION WITH_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
  SOURCES tests/perfsampler_test.cpp tests/perfsampler_test.h
)

extend_qtc_plugin(Profiler
  CONDITION NOT QTPROFILER_WASM
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
  # An installation brings its own elfutils, in lib/elfutils (see
  # scripts/deploy.py), for systems that have none.
  set_property(TARGET Profiler APPEND PROPERTY INSTALL_RPATH "\$ORIGIN/../../elfutils")
endif()
