import qbs 1.0
import qbs.File
import qbs.FileInfo

QtcPlugin {
    name: "Profiler"

    Properties {
        condition: project.qtprofilerWasm
        cpp.defines: base.concat("QTPROFILER_WASM")
    }

    Depends { name: "Qt"; submodules: ["widgets", "network"] }

    Depends { name: "CommonTraceFormat" }
    // Only the details rewriter parses QML, and it cannot resolve a source
    // file on WebAssembly (see the CMakeLists.txt).
    Depends {
        name: "QmlJS"
        condition: !project.qtprofilerWasm
    }
    Depends { name: "QmlDebug" }
    Depends { name: "Utils" }
    Depends { name: "Tracing"; required: false }
    Depends { name: "QtTaskTree" }
    Depends { name: "McpServerLib" }

    Depends { name: "Core" }
    // ProjectExplorer and QtSupport are only needed by the Qt Creator
    // integration, which a WebAssembly build leaves out (see CMakeLists.txt).
    Depends {
        name: "ProjectExplorer"
        condition: !project.qtprofilerWasm
    }
    Depends {
        name: "QtSupport"
        condition: !project.qtprofilerWasm
    }
    // TextEditor is only used for the text marks that annotate source in Qt
    // Creator's editors, which a WebAssembly build has none of.
    Depends {
        name: "TextEditor"
        condition: !project.qtprofilerWasm
    }

    // PerfDataReader (perfdatareader.cpp, backing the CPU Usage analyzer)
    // shells out to perfparser at runtime; building Profiler should build it
    // too. (The Perf Sampler backend in perfsampler.cpp does not use
    // perfparser -- it decodes "perf record"'s output itself, see
    // perfrecordreader.cpp.) Not required: the "Perf Parser" project is
    // Linux-only (see perfparser.qbs).
    Depends { name: "perfparser"; required: false }

    condition: Tracing.present
    // The Windows sampler backend is compiled on every Windows toolchain, so
    // the libraries it needs are linked for all of them.
    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: ["advapi32", "dbghelp", "psapi"]
    }

    // canSampleOtherProcesses() asks the Security framework what entitlements
    // this process was granted (see macsampler.cpp).
    Properties {
        condition: qbs.targetOS.contains("macos")
        cpp.frameworks: ["Security"]
    }

    Group {
        name: "details rewriter"
        condition: !qbs.toolchain.contains("emscripten")
        files: ["qmlprofilerdetailsrewriter.cpp"]
    }

    Group {
        name: "details rewriter (WebAssembly)"
        condition: project.qtprofilerWasm
        files: ["qmlprofilerdetailsrewriter_wasm.cpp"]
    }

    Group {
        name: "Qt Creator integration"
        condition: !project.qtprofilerWasm
        files: [
            "mcpsupport.cpp", "mcpsupport.h",
            "profilerplugin.cpp",
            "qmlprofilertool.cpp", "qmlprofilertool.h",
            "perfprofilertool.cpp", "perfprofilertool.h",
            "profilermode.cpp", "profilermode.h",
            "profilersamplerruncontrol.cpp", "profilersamplerruncontrol.h",
            "profilerstarteditor.cpp", "profilerstarteditor.h",
            "qmlprofilerattachdialog.cpp", "qmlprofilerattachdialog.h",
            "qmlprofilerrunconfigurationaspect.cpp", "qmlprofilerrunconfigurationaspect.h",
            "qmlprofilerruncontrol.cpp", "qmlprofilerruncontrol.h",
            "perfloaddialog.cpp", "perfloaddialog.h",
            "perfprofilerruncontrol.cpp", "perfprofilerruncontrol.h",
            "perfrunconfigurationaspect.cpp", "perfrunconfigurationaspect.h",
            "perftracepointdialog.cpp", "perftracepointdialog.h",
        ]
    }

    // Only part of the build on Windows: the sources use Windows APIs without
    // Q_OS_WIN guards.
    Group {
        name: "Windows sampler"
        condition: qbs.targetOS.contains("windows")
        files: [
            "etwlauncher_win.cpp", "etwlauncher_win.h",
            "winsampler.cpp", "winsampler.h",
            "winsymbolicator.cpp", "winsymbolicator.h",
        ]
    }

    // Optional: enables dwarf-mode call-graph unwinding in the Perf Sampler
    // backend (perfdwarfunwinder.cpp), via libdw's Dwfl_Thread_Callbacks API.
    // Mirrors perfparser.qbs's own (unrelated) ELFUTILS_INSTALL_DIR lookup.
    // libdw/libelf are dual-licensed LGPL-3.0-or-later / GPL-2.0-or-later;
    // the LGPL election is what makes linking them into this dual-licensed
    // plugin possible. No qt_attributions.json entry: that file covers
    // vendored third-party source shipped in this repo, not a
    // dynamically-linked system library. See CMakeLists.txt for the
    // equivalent CMake-side comment.
    Probe {
        id: elfutilsProbe
        property string installBase: qbs.getenv("ELFUTILS_INSTALL_DIR")
        property string includeDir: installBase
            ? FileInfo.joinPaths(installBase, "include")
            : "/usr/include"
        property string libDir: installBase ? FileInfo.joinPaths(installBase, "lib") : ""
        property bool found
        configure: {
            found = File.exists(FileInfo.joinPaths(includeDir, "elfutils", "libdwfl.h"));
        }
    }

    // PerfDwarfUnwinder handles x86-64 registers only, and the sampler is Linux-only.
    property bool withLibdw: elfutilsProbe.found && qbs.targetOS.contains("linux")
                             && qbs.architecture === "x86_64"

    cpp.includePaths: withLibdw
        ? base.concat([elfutilsProbe.includeDir,
                       FileInfo.joinPaths(elfutilsProbe.includeDir, "elfutils")])
        : base
    cpp.libraryPaths: withLibdw && elfutilsProbe.libDir
        ? base.concat([elfutilsProbe.libDir])
        : base
    cpp.dynamicLibraries: withLibdw ? base.concat(["dw", "elf"]) : base
    cpp.defines: withLibdw ? base.concat(["WITH_LIBDW"]) : base

    Group {
        name: "DwarfUnwinder"
        condition: withLibdw
        files: ["perfdwarfunwinder.cpp", "perfdwarfunwinder.h"]
    }

    Group {
        name: "General"
        files: [
            "ctfloader.cpp", "ctfloader.h",
            "callstacksampler.cpp", "callstacksampler.h",
            "calltreemodel.cpp", "calltreemodel.h",
            "combinedsampler.cpp", "combinedsampler.h",
            "combinedtraceloader.cpp", "combinedtraceloader.h",
            "calltreeview.cpp", "calltreeview.h",
            "cpuusagemodel.cpp", "cpuusagemodel.h",
            "macsampler.cpp", "macsampler.h",
            "processpickerdialog.cpp", "processpickerdialog.h",
            "qmlprofilersampler.cpp", "qmlprofilersampler.h",
            "qttracesampler.cpp", "qttracesampler.h",
            "samplemerge.cpp", "samplemerge.h",
            "sampler.cpp", "sampler.h",
            "samplerrecipe.cpp", "samplerrecipe.h",
            "samplertracebackend.cpp", "samplertracebackend.h",
            "samplerviewmanager.cpp", "samplerviewmanager.h",
            "sampletrace.cpp", "sampletrace.h",
            "symbolicator.cpp", "symbolicator.h",
            "ctfplainviewmanager.cpp", "ctfplainviewmanager.h",
            "ctfstatisticsmodel.cpp", "ctfstatisticsmodel.h",
            "ctftracebackend.cpp", "ctftracebackend.h",
            "ctfstatisticsview.cpp", "ctfstatisticsview.h",
            "ctftimelinemodel.cpp", "ctftimelinemodel.h",
            "ctftracemanager.cpp", "ctftracemanager.h",
            "ctfvisualizerconstants.h",
            "ctfvisualizertool.cpp", "ctfvisualizertool.h",
            "debugmessagesmodel.cpp", "debugmessagesmodel.h",
            "flamegraphmodel.cpp", "flamegraphmodel.h",
            "flamegraphview.cpp", "flamegraphview.h",
            "inputeventsmodel.cpp", "inputeventsmodel.h",
            "memoryusagemodel.cpp", "memoryusagemodel.h",
            "pixmapcachemodel.cpp", "pixmapcachemodel.h",
            "qmlnote.cpp", "qmlnote.h",
            "profilerrecorder.cpp", "profilerrecorder.h",
            "profilertracebackend.cpp", "profilertracebackend.h",
            "profilertracedocument.cpp", "profilertracedocument.h",
            "profilertraceeditor.cpp", "profilertraceeditor.h",
            "profilertr.h",
            "profiler_global.h",
            "recordingpage.cpp", "recordingpage.h",
            "traceformat.cpp", "traceformat.h",
            "welcomepage.cpp", "welcomepage.h",
            "qmlprofileranimationsmodel.h", "qmlprofileranimationsmodel.cpp",
            "qmlprofilerclientmanager.cpp", "qmlprofilerclientmanager.h",
            "qmlprofilerconstants.h",
            "qmlprofilerdashboardstats.cpp", "qmlprofilerdashboardstats.h",
            "qmlprofilerdashboardview.cpp", "qmlprofilerdashboardview.h",
            "qmlprofilerdetailsrewriter.h",
            "qmlprofilerfindings.cpp", "qmlprofilerfindings.h",
            "qmlprofilerfindingsmodel.cpp", "qmlprofilerfindingsmodel.h",
            "qmlprofilerfindingsview.cpp", "qmlprofilerfindingsview.h",
            "qmlprofilereventsview.h",
            "qmlprofilermodelmanager.cpp", "qmlprofilermodelmanager.h",
            "qmlprofilernotesmodel.cpp", "qmlprofilernotesmodel.h",
            "qmlprofilerplainviewmanager.cpp", "qmlprofilerplainviewmanager.h",
            "qmlprofilerrangemodel.cpp", "qmlprofilerrangemodel.h",
            "qmlprofilersettings.cpp", "qmlprofilersettings.h",
            "qmlprofilerstatemanager.cpp", "qmlprofilerstatemanager.h",
            "qmlprofilerstatewidget.cpp", "qmlprofilerstatewidget.h",
            "qmlprofilerstatisticsmodel.cpp", "qmlprofilerstatisticsmodel.h",
            "qmlprofilerstatisticsview.cpp", "qmlprofilerstatisticsview.h",
            "qmlprofilertimelinemodel.cpp", "qmlprofilertimelinemodel.h",
            "qmlprofilertracebackend.cpp", "qmlprofilertracebackend.h",
            "qmlprofilertracefile.cpp", "qmlprofilertracefile.h",
            "qmlprofilertraceview.cpp", "qmlprofilertraceview.h",
            "qmlrecordingtimeline.h",
            "quick3dmodel.cpp", "quick3dmodel.h",
            "quick3dframeview.cpp", "quick3dframeview.h",
            "quick3dframemodel.cpp", "quick3dframemodel.h",
            "scenegraphtimelinemodel.cpp", "scenegraphtimelinemodel.h",
        ]
    }

    // Formerly the separate PerfProfiler plugin, folded in. See
    // design-docs/native-mixed-profiler-design.md.
    Group {
        name: "Perf"
        files: [
            "dwarflinetable.cpp", "dwarflinetable.h",
            "perfconfigeventsmodel.cpp", "perfconfigeventsmodel.h",
            "perfdatareader.cpp", "perfdatareader.h",
            "perfevent.h",
            "perfeventtype.h",
            "perfnativemixed.cpp", "perfnativemixed.h",
            "perfprofilertr.h",
            "perfprofilerconstants.h",
            "perfprofilerflamegraphmodel.cpp", "perfprofilerflamegraphmodel.h",
            "perfprofilertracebackend.cpp", "perfprofilertracebackend.h",
            "perfprofilerflamegraphview.cpp", "perfprofilerflamegraphview.h",
            "perfprofilerstatisticsmodel.cpp", "perfprofilerstatisticsmodel.h",
            "perfprofilerstatisticsview.cpp", "perfprofilerstatisticsview.h",
            "perfprofilertracefile.cpp", "perfprofilertracefile.h",
            "perfprofilertracemanager.cpp", "perfprofilertracemanager.h",
            "perfrecordreader.cpp", "perfrecordreader.h",
            "perfresourcecounter.h",
            "perfsampler.cpp", "perfsampler.h",
            "perfsettings.cpp", "perfsettings.h",
            "perftimelinemodel.cpp", "perftimelinemodel.h",
            "perftimelinemodelmanager.cpp", "perftimelinemodelmanager.h",
            "perfprofiler.qrc",
            "profiler.qrc",
        ]
    }

    QtcTestFiles {
        prefix: "tests/"
        files: [
            "dwarflinetable_test.cpp", "dwarflinetable_test.h",
            "perfnativemixed_test.cpp", "perfnativemixed_test.h",
            "perfprofilertracefile_test.cpp", "perfprofilertracefile_test.h",
            "perfrecordreader_test.cpp", "perfrecordreader_test.h",
            "perfresourcecounter_test.cpp", "perfresourcecounter_test.h",
            "perfsampler_test.cpp", "perfsampler_test.h",
            "perfprofilertests.qrc",

            "calltreeview_test.cpp", "calltreeview_test.h",
            "ctfloader_test.cpp", "ctfloader_test.h",
            "ctftimelinemodel_test.cpp", "ctftimelinemodel_test.h",
            "debugmessagesmodel_test.cpp", "debugmessagesmodel_test.h",
            "fakedebugserver.cpp", "fakedebugserver.h",
            "flamegraphmodel_test.cpp", "flamegraphmodel_test.h",
            "flamegraphview_test.cpp", "flamegraphview_test.h",
            "inputeventsmodel_test.cpp", "inputeventsmodel_test.h",
            "localqmlprofilerrunner_test.cpp", "localqmlprofilerrunner_test.h",
            "memoryusagemodel_test.cpp", "memoryusagemodel_test.h",
            "pixmapcachemodel_test.cpp", "pixmapcachemodel_test.h",
            "profilertracedocument_test.cpp", "profilertracedocument_test.h",
            "qmlnote_test.cpp", "qmlnote_test.h",
            "qmlprofileranimationsmodel_test.cpp", "qmlprofileranimationsmodel_test.h",
            "qmlprofilerattachdialog_test.cpp", "qmlprofilerattachdialog_test.h",
            "qmlprofilerclientmanager_test.cpp", "qmlprofilerclientmanager_test.h",
            "qmlprofilerdetailsrewriter_test.cpp", "qmlprofilerdetailsrewriter_test.h",
            "qmlprofilerfindingsmodel_test.cpp", "qmlprofilerfindingsmodel_test.h",
            "qmlprofilersampler_test.cpp", "qmlprofilersampler_test.h",
            "qmlprofilertool_test.cpp", "qmlprofilertool_test.h",
            "qmlprofilertracefile_test.cpp", "qmlprofilertracefile_test.h",
            "qmlprofilertraceview_test.cpp", "qmlprofilertraceview_test.h",
            "qttracesampler_test.cpp", "qttracesampler_test.h",

            "tests.qrc"
        ]
    }
}
