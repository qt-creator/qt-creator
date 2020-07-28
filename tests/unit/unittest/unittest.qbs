import qbs.File
import qbs.FileInfo

QtcProduct {
    condition: gtest.present && gmock.present
    type: ["application", "autotest", "json_copy"]
    consoleApplication: true
    destinationDirectory: FileInfo.joinPaths(project.buildDirectory,
        FileInfo.relativePath(project.ide_source_tree, sourceDirectory))
    install: false

    Depends { name: "echoserver" }
    Depends { name: "pluginjson" }
    Depends { name: "libclang"; required: false }
    Depends { name: "clang_defines" }

    Depends { name: "QmlDesigner" }

    Depends { name: "sqlite_sources" }
    Depends { name: "Core" }
    Depends { name: "CPlusPlus" }
    Depends { name: "yaml-cpp" }

    Depends { name: "Qt"; submodules: ["network", "widgets", "testlib"] }

    Depends { name: "pkgconfig"; required: false }
    Depends { name: "benchmark"; required: false }
    Depends { name: "gtest"; required: false }
    Depends { name: "gmock"; required: false }

    sqlite_sources.buildSharedLib: false

    cpp.defines: {
        var defines = [
            "QT_NO_CAST_TO_ASCII",
            "QT_RESTRICTED_CAST_FROM_ASCII",
            "QT_USE_FAST_OPERATOR_PLUS",
            "QT_USE_FAST_CONCATENATION",
            "CLANG_UNIT_TESTS",
            "UNIT_TESTS",
            "DONT_CHECK_MESSAGE_COUNTER",
            'QTC_RESOURCE_DIR="' + path + "/../../../share/qtcreator" + '"',
            'TESTDATA_DIR="' + FileInfo.joinPaths(sourceDirectory, "data") + '"',
            'ECHOSERVER="' + FileInfo.joinPaths(project.buildDirectory, "tests", "unit",
                                                "echoserver", "echo") + '"',
            'RELATIVE_DATA_PATH="' + FileInfo.relativePath(destinationDirectory,
                FileInfo.joinPaths(project.sourceDirectory, "share", "qtcreator")) + '"',
            'CPPTOOLS_JSON="' + FileInfo.joinPaths(destinationDirectory, "CppTools.json") + '"',
        ];
        if (libclang.present && libclang.toolingEnabled)
            defines = defines.concat(libclang.llvmToolingDefines);
        return defines;
    }
    cpp.cxxFlags: {
        var flags = [];
        if (qbs.toolchain.contains("msvc"))
           flags.push("-w34100", "/bigobj", "/wd4267", "/wd4141", "/wd4146");
        if (qbs.toolchain.contains("gcc") && !qbs.toolchain.contains("clang"))
            flags.push("-Wno-noexcept-type");
        if (qbs.toolchain.contains("clang")) {
            flags.push("-Wno-inconsistent-missing-override", "-Wno-self-move",
                       "-Wno-self-assign-overloaded");
            flags.push("-Wno-unused-command-line-argument"); // gtest puts -lpthread on compiler command line
            if (!qbs.hostOS.contains("darwin")
                    && Utilities.versionCompare(cpp.compilerVersion, "10") >= 0) {
                flags.push("-Wno-deprecated-copy", "-Wno-constant-logical-operand");
            }
        }
        if (qbs.toolchain.contains("gcc"))
            flags.push("-Wno-unused-parameter");
        if (libclang.present && libclang.toolingEnabled)
            flags = flags.concat(libclang.llvmToolingCxxFlags);
        return flags;
    }
    cpp.cxxLanguageVersion: "c++17"
    cpp.dynamicLibraries: {
        var libs = [];
        if (libclang.present) {
            libs = libs.concat(libclang.llvmLibs);
            if (libclang.toolingEnabled)
                libs = libs.concat(libclang.llvmToolingLibs);
            if (libclang.llvmFormattingLibs.length
                    && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)) {
                libs = libs.concat(libclang.llvmFormattingLibs);
            }
        }
        return libs;
    }
    cpp.includePaths: {
        var paths = [
            ".",
            "../mockup",
            "../../../src/libs",
            "../../../src/libs/3rdparty",
            "../../../src/libs/clangsupport",
            "../../../src/plugins",
            "../../../src/plugins/clangcodemodel",
            "../../../src/plugins/clangpchmanager",
            "../../../src/plugins/clangrefactoring",
            "../../../src/tools/clangbackend/source",
            "../../../src/tools/clangpchmanagerbackend/source",
            "../../../src/tools/clangrefactoringbackend/source",
            "../../../share/qtcreator/qml/qmlpuppet/types",
        ];
        if (libclang.present) {
            paths.push(libclang.llvmIncludeDir);
            if (libclang.toolingEnabled)
                paths = paths.concat(libclang.llvmToolingIncludes);
        }
        return paths;
    }
    cpp.libraryPaths: {
        var paths = [];
        if (libclang.present)
            paths.push(libclang.llvmLibDir);
        return paths;
    }
    cpp.rpaths: {
        var paths = [
            FileInfo.joinPaths(project.buildDirectory, qtc.ide_library_path),
            FileInfo.joinPaths(project.buildDirectory, qtc.ide_plugin_path)
        ];
        if (libclang.present)
            paths.push(libclang.llvmLibDir);
        return paths;
    }

    files: [
        "builddependenciesprovider-test.cpp",
        "builddependenciesstorage-test.cpp",
        "clangindexingsettingsmanager-test.cpp",
        "clangpathwatcher-test.cpp",
        "clangqueryexamplehighlightmarker-test.cpp",
        "clangqueryhighlightmarker-test.cpp",
        "clientserverinprocess-test.cpp",
        "clientserveroutsideprocess-test.cpp",
        "commandlinebuilder-test.cpp",
        "compare-operators.h",
        "compilationdatabaseutils-test.cpp",
        "compileroptionsbuilder-test.cpp",
        "conditionally-disabled-tests.h",
        "cppprojectfilecategorizer-test.cpp",
        "cppprojectinfogenerator-test.cpp",
        "cppprojectpartchooser-test.cpp",
        "createtablesqlstatementbuilder-test.cpp",
        "directorypathcompressor-test.cpp",
        "dummyclangipcclient.h",
        "dynamicastmatcherdiagnosticcontainer-matcher.h",
        "eventspy.cpp",
        "eventspy.h",
        "fakeprocess.cpp",
        "fakeprocess.h",
        "filepath-test.cpp",
        "filepathcache-test.cpp",
        "filepathstorage-test.cpp",
        "filepathstoragesqlitestatementfactory-test.cpp",
        "filepathview-test.cpp",
        "filestatuscache-test.cpp",
        "filesystem-utilities.h",
        "generatedfiles-test.cpp",
        "google-using-declarations.h",
        "googletest.h",
        "gtest-creator-printing.cpp",
        "gtest-creator-printing.h",
        "gtest-llvm-printing.h",
        "gtest-qt-printing.cpp",
        "gtest-qt-printing.h",
        "headerpathfilter-test.cpp",
        "lineprefixer-test.cpp",
        "locatorfilter-test.cpp",
        "matchingtext-test.cpp",
        "mimedatabase-utilities.cpp",
        "mimedatabase-utilities.h",
        "mockbuilddependenciesprovider.h",
        "mockbuilddependenciesstorage.h",
        "mockbuilddependencygenerator.h",
        "mockclangcodemodelclient.h",
        "mockclangcodemodelserver.h",
        "mockclangpathwatcher.h",
        "mockclangpathwatchernotifier.h",
        "mockcppmodelmanager.h",
        "mockeditormanager.h",
        "mockfilepathcaching.h",
        "mockfilepathstorage.h",
        "mockfilesystem.h",
        "mockfutureinterface.h",
        "mockgeneratedfiles.h",
        "mockmodifiedtimechecker.h",
        "mockmutex.h",
        "mockpchcreator.h",
        "mockpchmanagerclient.h",
        "mockpchmanagernotifier.h",
        "mockpchmanagerserver.h",
        "mockpchtaskgenerator.h",
        "mockpchtaskqueue.h",
        "mockpchtasksmerger.h",
        "mockprecompiledheaderstorage.h",
        "mockprocessor.h",
        "mockprocessormanager.h",
        "mockprogressmanager.h",
        "mockprojectpartprovider.h",
        "mockprojectpartqueue.h",
        "mockprojectpartsmanager.h",
        "mockprojectpartsstorage.h",
        "mockqfilesystemwatcher.h",
        "mockqueue.h",
        "mocksearch.h",
        "mocksearchhandle.h",
        "mocksearchresult.h",
        "mocksqlitedatabase.h",
        "mocksqlitereadstatement.cpp",
        "mocksqlitereadstatement.h",
        "mocksqlitestatement.h",
        "mocksqlitetransactionbackend.h",
        "mocksqlitewritestatement.h",
        "mocksymbolindexertaskqueue.h",
        "mocksymbolindexing.h",
        "mocksymbolquery.h",
        "mocksymbolscollector.h",
        "mocksymbolstorage.h",
        "mocksyntaxhighligher.h",
        "mocktaskscheduler.h",
        "mocktimer.cpp",
        "mocktimer.h",
        "modifiedtimechecker-test.cpp",
        "nativefilepath-test.cpp",
        "nativefilepathview-test.cpp",
        "pchmanagerclient-test.cpp",
        "pchmanagerclientserverinprocess-test.cpp",
        "pchmanagerserver-test.cpp",
        "pchtaskgenerator-test.cpp",
        "pchtaskqueue-test.cpp",
        "pchtasksmerger-test.cpp",
        "precompiledheaderstorage-test.cpp",
        "preprocessormacrocollector-test.cpp",
        "processcreator-test.cpp",
        "processevents-utilities.cpp",
        "processevents-utilities.h",
        "processormanager-test.cpp",
        "progresscounter-test.cpp",
        "projectpartartefact-test.cpp",
        "projectpartsmanager-test.cpp",
        "projectpartsstorage-test.cpp",
        "projectupdater-test.cpp",
        "readandwritemessageblock-test.cpp",
        "refactoringdatabaseinitializer-test.cpp",
        "refactoringprojectupdater-test.cpp",
        "rundocumentparse-utility.h",
        "sizedarray-test.cpp",
        "smallstring-test.cpp",
        "sourcerangecontainer-matcher.h",
        "sourcerangefilter-test.cpp",
        "sourcesmanager-test.cpp",
        "spydummy.cpp",
        "spydummy.h",
        "sqlitecolumn-test.cpp",
        "sqlitedatabase-test.cpp",
        "sqlitedatabasebackend-test.cpp",
        "sqliteindex-test.cpp",
        "sqlitestatement-test.cpp",
        "sqlitetable-test.cpp",
        "sqliteteststatement.h",
        "sqlitetransaction-test.cpp",
        "sqlitevalue-test.cpp",
        "sqlstatementbuilder-test.cpp",
        "stringcache-test.cpp",
        "symbolindexer-test.cpp",
        "symbolindexertaskqueue-test.cpp",
        "symbolquery-test.cpp",
        "symbolsfindfilter-test.cpp",
        "symbolstorage-test.cpp",
        "task.cpp",
        "taskscheduler-test.cpp",
        "testenvironment.h",
        "toolchainargumentscache-test.cpp",
        "unittest-utility-functions.h",
        "unittests-main.cpp",
        "usedmacrofilter-test.cpp",
        "utf8-test.cpp",
    ]

    Group {
        name: "libclang tests"
        condition: libclang.present
        files: [
            "activationsequencecontextprocessor-test.cpp",
            "activationsequenceprocessor-test.cpp",
            "chunksreportedmonitor.cpp",
            "chunksreportedmonitor.h",
            "clangasyncjob-base.cpp",
            "clangasyncjob-base.h",
            "clangcodecompleteresults-test.cpp",
            "clangcodemodelserver-test.cpp",
            "clangcompareoperators.h",
            "clangcompletecodejob-test.cpp",
            "clangcompletioncontextanalyzer-test.cpp",
            "clangdiagnosticfilter-test.cpp",
            "clangdocument-test.cpp",
            "clangdocumentprocessor-test.cpp",
            "clangdocumentprocessors-test.cpp",
            "clangdocuments-test.cpp",
            "clangfixitoperation-test.cpp",
            "clangfollowsymbol-test.cpp",
            "clangisdiagnosticrelatedtolocation-test.cpp",
            "clangjobqueue-test.cpp",
            "clangjobs-test.cpp",
            "clangparsesupportivetranslationunitjob-test.cpp",
            "clangrequestannotationsjob-test.cpp",
            "clangrequestreferencesjob-test.cpp",
            "clangresumedocumentjob-test.cpp",
            "clangstring-test.cpp",
            "clangsupportivetranslationunitinitializer-test.cpp",
            "clangsuspenddocumentjob-test.cpp",
            "clangtooltipinfo-test.cpp",
            "clangtranslationunit-test.cpp",
            "clangtranslationunits-test.cpp",
            "clangupdateannotationsjob-test.cpp",
            "codecompleter-test.cpp",
            "codecompletionsextractor-test.cpp",
            "completionchunkstotextconverter-test.cpp",
            "cursor-test.cpp",
            "diagnostic-test.cpp",
            "diagnosticcontainer-matcher.h",
            "diagnosticset-test.cpp",
            "fixit-test.cpp",
            "highlightingresultreporter-test.cpp",
            "readexporteddiagnostics-test.cpp",
            "senddocumenttracker-test.cpp",
            "skippedsourceranges-test.cpp",
            "sourcelocation-test.cpp",
            "sourcerange-test.cpp",
            "token-test.cpp",
            "translationunitupdater-test.cpp",
            "unsavedfile-test.cpp",
            "unsavedfiles-test.cpp",
            "utf8positionfromlinecolumn-test.cpp",
        ]
    }

    Group {
        name: "clang tooling tests"
        condition: libclang.present && libclang.toolingEnabled
        files: [
            "builddependencycollector-test.cpp",
            "clangdocumentsuspenderresumer-test.cpp",
            "clangquery-test.cpp",
            "clangquerygatherer-test.cpp",
            "clangqueryprojectfindfilter-test.cpp",
            "clangreferencescollector-test.cpp",
            "gtest-clang-printing.cpp",
            "gtest-clang-printing.h",
            "gtest-llvm-printing.cpp",
            "mockrefactoringclient.h",
            "mockrefactoringserver.h",
            "pchcreator-test.cpp",
            "refactoringclient-test.cpp",
            "refactoringclientserverinprocess-test.cpp",
            "refactoringcompilationdatabase-test.cpp",
            "refactoringengine-test.cpp",
            "refactoringserver-test.cpp",
            "sourcerangeextractor-test.cpp",
            "symbolindexing-test.cpp",
            "symbolscollector-test.cpp",
            "testclangtool.cpp",
            "testclangtool.h",
            "tokenprocessor-test.cpp",
            "usedmacrocollector-test.cpp",
        ]
    }

    Group {
        name: "ClangFormat tests"
        condition: libclang.present
                   && libclang.llvmFormattingLibs.length
                   && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)
        files: "clangformat-test.cpp"
    }

    Group {
        name: "benchmark test"
        condition: benchmark.present
        files: "smallstring-benchmark.cpp"
    }

    Group {
        name: "data"
        files: [
            "data/*",
            "data/include/*",
        ]
        fileTags: []
    }

    Group {
        name: "json.in file"
        files: "../../../src/plugins/cpptools/CppTools.json.in"
        fileTags: "pluginJsonIn"
    }

    Group {
        name: "sources from pchmanager"
        prefix: "../../../src/plugins/clangpchmanager/"
        cpp.defines: outer.concat("CLANGPCHMANAGER_STATIC_LIB")
        files: [
            "clangindexingprojectsettings.cpp",
            "clangindexingprojectsettings.h",
            "clangindexingsettingsmanager.cpp",
            "clangindexingsettingsmanager.h",
            "clangpchmanager_global.h",
            "pchmanagerclient.cpp",
            "pchmanagerclient.h",
            "pchmanagerconnectionclient.cpp",
            "pchmanagerconnectionclient.h",
            "pchmanagernotifierinterface.cpp",
            "pchmanagernotifierinterface.h",
            "pchmanagerprojectupdater.cpp",
            "pchmanagerprojectupdater.h",
            "preprocessormacrocollector.cpp",
            "preprocessormacrocollector.h",
            "progressmanager.h",
            "progressmanagerinterface.h",
            "projectupdater.cpp",
            "projectupdater.h",
        ]
    }

    Group {
        name: "sources from pchmanager backend"
        prefix: "../../../src/tools/clangpchmanagerbackend/source/"
        files: [
            "builddependenciesprovider.cpp",
            "builddependenciesprovider.h",
            "builddependenciesproviderinterface.h",
            "builddependenciesstorage.h",
            "builddependenciesstorageinterface.h",
            "builddependency.h",
            "builddependencygeneratorinterface.h",
            "clangpchmanagerbackend_global.h",
            "generatepchactionfactory.h",
            "pchcreatorinterface.h",
            "pchmanagerserver.cpp",
            "pchmanagerserver.h",
            "pchnotcreatederror.h",
            "pchtask.h",
            "pchtaskgenerator.cpp",
            "pchtaskgenerator.h",
            "pchtaskgeneratorinterface.h",
            "pchtaskqueue.cpp",
            "pchtaskqueue.h",
            "pchtaskqueueinterface.h",
            "pchtasksmerger.cpp",
            "pchtasksmerger.h",
            "pchtasksmergerinterface.h",
            "precompiledheaderstorage.h",
            "precompiledheaderstorageinterface.h",
            "processorinterface.h",
            "processormanagerinterface.h",
            "projectpartsmanager.cpp",
            "projectpartsmanager.h",
            "projectpartsmanagerinterface.h",
            "queueinterface.h",
            "taskscheduler.h",
            "taskschedulerinterface.h",
            "toolchainargumentscache.h",
            "usedmacrofilter.h",
        ]

        Group {
            name: "tooling sources from pchmanager backend"
            condition: libclang.toolingEnabled
            files: [
                "builddependencycollector.cpp",
                "builddependencycollector.h",
                "collectbuilddependencyaction.h",
                "collectbuilddependencypreprocessorcallbacks.h",
                "collectbuilddependencytoolaction.h",
                "collectusedmacroactionfactory.h",
                "collectusedmacrosaction.h",
                "collectusedmacrosandsourcespreprocessorcallbacks.h",
                "pchcreator.cpp",
                "pchcreator.h",
                "processormanager.h",
                "usedmacrosandsourcescollector.cpp",
                "usedmacrosandsourcescollector.h",
            ]
        }
    }

    Group {
        name: "sources from clangrefactoring backend"
        prefix: "../../../src/tools/clangrefactoringbackend/source/"
        files: [
            "clangrefactoringbackend_global.h",
            "collectmacrospreprocessorcallbacks.h",
            "projectpartentry.h",
            "sourcedependency.h",
            "sourcelocationentry.h",
            "sourcerangefilter.cpp",
            "sourcerangefilter.h",
            "sourcesmanager.h",
            "symbolentry.h",
            "symbolindexer.cpp",
            "symbolindexer.h",
            "symbolindexertask.h",
            "symbolindexertaskqueue.h",
            "symbolindexertaskqueueinterface.h",
            "symbolindexing.h",
            "symbolindexinginterface.h",
            "symbolscollectorinterface.h",
            "symbolstorage.h",
            "symbolstorageinterface.h",
            "usedmacro.h",
        ]

        Group {
            name: "tooling sources from clangrefactoring backend"
            condition: libclang.toolingEnabled
            files: [
                "clangquery.cpp",
                "clangquery.h",
                "clangquerygatherer.cpp",
                "clangquerygatherer.h",
                "clangtool.cpp",
                "clangtool.h",
                "collectmacrossourcefilecallbacks.cpp",
                "collectmacrossourcefilecallbacks.h",
                "collectsymbolsaction.cpp",
                "collectsymbolsaction.h",
                "indexdataconsumer.cpp",
                "indexdataconsumer.h",
                "locationsourcefilecallbacks.cpp",
                "locationsourcefilecallbacks.h",
                "macropreprocessorcallbacks.cpp",
                "macropreprocessorcallbacks.h",
                "refactoringcompilationdatabase.cpp",
                "refactoringcompilationdatabase.h",
                "refactoringserver.cpp",
                "refactoringserver.h",
                "sourcelocationsutils.h",
                "sourcerangeextractor.cpp",
                "sourcerangeextractor.h",
                "symbolindexing.cpp",
                "symbolscollector.cpp",
                "symbolscollector.h",
                "symbolsvisitorbase.h",
            ]
        }
    }

    Group {
        name: "sources from clangbackend"
        condition: libclang.present
        prefix: "../../../src/tools/clangbackend/source/"
        files: [
            "clangasyncjob.h",
            "clangbackend_global.h",
            "clangclock.h",
            "clangcodecompleteresults.cpp",
            "clangcodecompleteresults.h",
            "clangcodemodelserver.cpp",
            "clangcodemodelserver.h",
            "clangcompletecodejob.cpp",
            "clangcompletecodejob.h",
            "clangdocument.cpp",
            "clangdocument.h",
            "clangdocumentjob.h",
            "clangdocumentprocessor.cpp",
            "clangdocumentprocessor.h",
            "clangdocumentprocessors.cpp",
            "clangdocumentprocessors.h",
            "clangdocuments.cpp",
            "clangdocuments.h",
            "clangdocumentsuspenderresumer.cpp",
            "clangdocumentsuspenderresumer.h",
            "clangexceptions.cpp",
            "clangexceptions.h",
            "clangfilepath.cpp",
            "clangfilepath.h",
            "clangfilesystemwatcher.cpp",
            "clangfilesystemwatcher.h",
            "clangfollowsymbol.cpp",
            "clangfollowsymbol.h",
            "clangfollowsymboljob.cpp",
            "clangfollowsymboljob.h",
            "clangiasyncjob.cpp",
            "clangiasyncjob.h",
            "clangjobcontext.cpp",
            "clangjobcontext.h",
            "clangjobqueue.cpp",
            "clangjobqueue.h",
            "clangjobrequest.cpp",
            "clangjobrequest.h",
            "clangjobs.cpp",
            "clangjobs.h",
            "clangparsesupportivetranslationunitjob.cpp",
            "clangparsesupportivetranslationunitjob.h",
            "clangreferencescollector.cpp",
            "clangreferencescollector.h",
            "clangrequestannotationsjob.cpp",
            "clangrequestannotationsjob.h",
            "clangrequestreferencesjob.cpp",
            "clangrequestreferencesjob.h",
            "clangrequesttooltipjob.cpp",
            "clangrequesttooltipjob.h",
            "clangresumedocumentjob.cpp",
            "clangresumedocumentjob.h",
            "clangstring.h",
            "clangsupportivetranslationunitinitializer.cpp",
            "clangsupportivetranslationunitinitializer.h",
            "clangsuspenddocumentjob.cpp",
            "clangsuspenddocumentjob.h",
            "clangtooltipinfocollector.cpp",
            "clangtooltipinfocollector.h",
            "clangtranslationunit.cpp",
            "clangtranslationunit.h",
            "clangtranslationunits.cpp",
            "clangtranslationunits.h",
            "clangtranslationunitupdater.cpp",
            "clangtranslationunitupdater.h",
            "clangtype.cpp",
            "clangtype.h",
            "clangunsavedfilesshallowarguments.cpp",
            "clangunsavedfilesshallowarguments.h",
            "clangupdateannotationsjob.cpp",
            "clangupdateannotationsjob.h",
            "clangupdateextraannotationsjob.cpp",
            "clangupdateextraannotationsjob.h",
            "codecompleter.cpp",
            "codecompleter.h",
            "codecompletionchunkconverter.cpp",
            "codecompletionchunkconverter.h",
            "codecompletionsextractor.cpp",
            "codecompletionsextractor.h",
            "commandlinearguments.cpp",
            "commandlinearguments.h",
            "cursor.cpp",
            "cursor.h",
            "diagnostic.cpp",
            "diagnostic.h",
            "diagnosticset.cpp",
            "diagnosticset.h",
            "diagnosticsetiterator.h",
            "fixit.cpp",
            "fixit.h",
            "fulltokeninfo.cpp",
            "fulltokeninfo.h",
            "skippedsourceranges.cpp",
            "skippedsourceranges.h",
            "sourcelocation.cpp",
            "sourcelocation.h",
            "sourcerange.cpp",
            "sourcerange.h",
            "token.cpp",
            "token.h",
            "tokeninfo.cpp",
            "tokeninfo.h",
            "tokenprocessor.h",
            "tokenprocessoriterator.h",
            "unsavedfile.cpp",
            "unsavedfile.h",
            "unsavedfiles.cpp",
            "unsavedfiles.h",
            "utf8positionfromlinecolumn.cpp",
            "utf8positionfromlinecolumn.h",
        ]
    }

    Group {
        name: "sources from clangsupport"
        prefix: "../../../src/libs/clangsupport/"
        cpp.defines: outer.concat("CLANGSUPPORT_STATIC_LIB")
        files: [
            "*.cpp",
            "*.h",
        ]
    }

    Group {
        name: "sources from clangcodemodel"
        prefix: "../../../src/plugins/clangcodemodel/"
        files: [
            "clangactivationsequencecontextprocessor.cpp",
            "clangactivationsequencecontextprocessor.h",
            "clangactivationsequenceprocessor.cpp",
            "clangactivationsequenceprocessor.h",
            "clangcompletionchunkstotextconverter.cpp",
            "clangcompletionchunkstotextconverter.h",
            "clangcompletioncontextanalyzer.cpp",
            "clangcompletioncontextanalyzer.h",
            "clangdiagnosticfilter.cpp",
            "clangdiagnosticfilter.h",
            "clangfixitoperation.cpp",
            "clangfixitoperation.h",
            "clanghighlightingresultreporter.cpp",
            "clanghighlightingresultreporter.h",
            "clangisdiagnosticrelatedtolocation.h",
            "clanguiheaderondiskmanager.cpp",
            "clanguiheaderondiskmanager.h",
        ]
    }

    Group {
        name: "sources from cpptools"
        prefix: "../../../src/plugins/cpptools/"
        cpp.defines: outer.concat("CPPTOOLS_STATIC_LIBRARY")
        files: [
            "compileroptionsbuilder.cpp",
            "compileroptionsbuilder.h",
            "cppprojectfile.cpp",
            "cppprojectfile.h",
            "cppprojectfilecategorizer.cpp",
            "cppprojectfilecategorizer.h",
            "cppprojectinfogenerator.cpp",
            "cppprojectpartchooser.cpp",
            "cppprojectpartchooser.h",
            "headerpathfilter.cpp",
            "headerpathfilter.h",
            "projectinfo.cpp",
            "projectinfo.h",
            "projectpart.cpp",
            "projectpart.h",
            "senddocumenttracker.cpp",
            "senddocumenttracker.h",
        ]
    }

    Group {
        name: "sources from clangtools"
        condition: libclang.present
        prefix: "../../../src/plugins/clangtools/"
        cpp.defines: outer.concat("CLANGTOOLS_STATIC_LIBRARY")
        files: [
            "clangtoolsdiagnostic.cpp",
            "clangtoolsdiagnostic.h",
            "clangtoolslogfilereader.cpp",
            "clangtoolslogfilereader.h",
        ]
    }

    Group {
        name: "sources from clangdbpm"
        prefix: "../../../src/plugins/compilationdatabaseprojectmanager/"
        files: [
            "compilationdatabaseutils.cpp",
            "compilationdatabaseutils.h",
        ]
    }

    Group {
        name: "sources from ProjectExplorer"
        prefix: "../../../src/plugins/projectexplorer/"
        cpp.defines: base.concat("PROJECTEXPLORER_STATIC_LIBRARY")
        files: [
            "projectmacro.cpp",
            "projectmacro.h",
        ]
    }

    Group {
        name: "sources from ClangRefactoring"
        prefix: "../../../src/plugins/clangrefactoring/"
        files: [
            "clangqueryexamplehighlighter.cpp",
            "clangqueryexamplehighlighter.h",
            "clangqueryexamplehighlightmarker.h",
            "clangqueryhighlighter.cpp",
            "clangqueryhighlighter.h",
            "clangqueryhighlightmarker.h",
            "clangqueryprojectsfindfilter.cpp",
            "clangqueryprojectsfindfilter.h",
            "clangsymbolsfindfilter.cpp",
            "clangsymbolsfindfilter.h",
            "editormanagerinterface.h",
            "locatorfilter.cpp",
            "locatorfilter.h",
            "projectpartproviderinterface.h",
            "projectpartutilities.cpp",
            "projectpartutilities.h",
            "refactoringclient.cpp",
            "refactoringclient.h",
            "refactoringconnectionclient.cpp",
            "refactoringconnectionclient.h",
            "refactoringengine.cpp",
            "refactoringengine.h",
            "refactoringprojectupdater.cpp",
            "refactoringprojectupdater.h",
            "searchhandle.cpp",
            "searchhandle.h",
            "searchinterface.h",
            "symbol.h",
            "symbolqueryinterface.h",
        ]
    }

    Group {
        name: "sources from ClangFormat"
        prefix: "../../../src/plugins/clangformat/"
        condition: libclang.present
                   && libclang.llvmFormattingLibs.length
                   && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)
        files: [
            "clangformatbaseindenter.cpp",
            "clangformatbaseindenter.h",
            "clangformatconstants.h",
        ]
    }

    Group {
        name: "sources from Debugger"
        prefix: "../../../src/plugins/debugger/analyzer/"
        cpp.defines: outer.concat("DEBUGGER_STATIC_LIBRARY")
        files: [
            "diagnosticlocation.cpp",
            "diagnosticlocation.h",
        ]
    }

    Rule {
        inputs: "qt_plugin_metadata"
        Artifact {
            filePath: FileInfo.joinPaths(product.destinationDirectory, "CppTools.json")
            fileTags: "json_copy"
        }
        prepare: {
            var cmd = new JavaScriptCommand;
            cmd.description = "copying " + input.fileName;
            cmd.sourceCode = function() { File.copy(input.filePath, output.filePath); };
            return cmd;
        }
    }

}
