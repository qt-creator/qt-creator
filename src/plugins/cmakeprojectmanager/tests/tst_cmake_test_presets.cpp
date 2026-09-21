// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include "../presetsmacros.h"
#include "../presetsparser.h"
#include "../testpresetshelper.h"

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace CMakeProjectManager::Internal::CMakePresets::Macros;

using namespace CMakePresets::Macros;
using namespace Utils;

class TestPresetsTests : public QObject
{
    Q_OBJECT

private slots:

    void testParseTestPresetsMinimal()
    {
        // Create a JSON containing only the minimal fields
        QJsonObject root;
        QJsonArray arr;
        QJsonObject obj;
        obj["name"] = "minimal";
        arr.append(obj);
        root["version"] = 5;
        root["testPresets"] = arr;

        QFile file(QDir::tempPath() + "/minimal.json");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(root).toJson());
        file.close();

        PresetsParser parser;
        QString error;
        int errorLine{0};
        bool ok = parser.parse(Utils::FilePath::fromUserInput(file.fileName()), error, errorLine);
        QVERIFY(ok);
        QVERIFY(error.isEmpty());

        const auto &data = parser.presetsData();
        QCOMPARE(data.testPresets.size(), 1);
        const auto &tp = data.testPresets.at(0);
        QCOMPARE(tp.name, QString("minimal"));
        QCOMPARE(tp.hidden, false);
        QCOMPARE(tp.fileDir.fileName(), QFileInfo(file.fileName()).dir().dirName());
        QVERIFY(!tp.inherits);
        QVERIFY(!tp.condition);
        QVERIFY(!tp.vendor);
        QVERIFY(!tp.displayName);
        QVERIFY(!tp.description);
        QVERIFY(!tp.environment);
        QVERIFY(!tp.configurePreset);
        QVERIFY(!tp.inheritConfigureEnvironment);
        QVERIFY(!tp.configuration);
        QVERIFY(!tp.overwriteConfigurationFile);
        QVERIFY(!tp.output);
        QVERIFY(!tp.filter);
        QVERIFY(!tp.execution);
    }

    void testParseTestPresetsFull()
    {
        const QJsonObject root = QJsonDocument::fromJson(R"(
        {
            "version": 5,
            "testPresets": [
                {
                    "name": "full",
                    "hidden": true,
                    "inherits": ["base1", "base2"],
                    "condition": {
                        "type": "matches",
                        "string": "Darwin"
                    },
                    "vendor": {
                        "qt.io/QtCreator/1.0": {
                            "debugger": "/path/to/dbg"
                        }
                    },
                    "displayName": "Full Test",
                    "description": "A test preset with all fields",
                    "environment": {
                        "PATH": "/custom/bin"
                    },
                    "configurePreset": "config1",
                    "inheritConfigureEnvironment": false,
                    "configuration": "Debug",
                    "overwriteConfigurationFile": ["CMakeLists.txt"],
                    "output": {
                        "shortProgress": true,
                        "verbosity": "high",
                        "debug": false,
                        "outputOnFailure": true,
                        "quiet": false,
                        "outputLogFile": "log.txt",
                        "outputJUnitFile": "results.xml",
                        "labelSummary": true,
                        "subprojectSummary": false,
                        "maxPassedTestOutputSize": 1024,
                        "maxFailedTestOutputSize": 2048,
                        "testOutputTruncation": "truncate",
                        "maxTestNameWidth": 80
                    },
                    "filter": {
                        "include": {
                            "name": "TestA",
                            "label": "unit",
                            "useUnion": true,
                            "index": {
                                "start": 1,
                                "end": 10,
                                "stride": 2,
                                "specificTests": [1, 3, 5]
                            }
                        },
                        "exclude": {
                            "name": "TestB",
                            "label": "integration",
                            "fixtures": {
                                "any": "BaseFixture",
                                "setup": "SetupFixture",
                                "cleanup": "CleanupFixture"
                            }
                        }
                    },
                    "execution": {
                        "stopOnFailure": true,
                        "enableFailover": false,
                        "jobs": 4,
                        "resourceSpecFile": "resources.json",
                        "testLoad": 2,
                        "showOnly": "TestA",
                        "repeat": {
                            "mode": "fixed",
                            "count": 3
                        },
                        "interactiveDebugging": true,
                        "scheduleRandom": false,
                        "timeout": 120,
                        "noTestsAction": "skip"
                    }
                }
            ]
        }
        )").object();


        QFile file(QDir::tempPath() + "/full.json");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(root).toJson());
        file.close();

        PresetsParser parser;
        QString error;
        int errorLine;
        bool ok = parser.parse(Utils::FilePath::fromUserInput(file.fileName()), error, errorLine);
        QVERIFY(ok);
        QVERIFY(error.isEmpty());

        const auto &data = parser.presetsData();
        QCOMPARE(data.testPresets.size(), 1);
        const auto &tp = data.testPresets.at(0);
        QCOMPARE(tp.name, QString("full"));
        QCOMPARE(tp.hidden, true);
        QCOMPARE(tp.inherits, QStringList() << "base1" << "base2");
        QVERIFY(tp.condition);
        QCOMPARE(tp.condition->isMatches(), true);
        QCOMPARE(tp.condition->string, "Darwin");
        QVERIFY(tp.vendor);
        QCOMPARE(tp.vendor->value("debugger").toString(), "/path/to/dbg");
        QCOMPARE(tp.displayName, QString("Full Test"));
        QCOMPARE(tp.description, QString("A test preset with all fields"));
        QVERIFY(tp.environment);
        QCOMPARE(tp.environment->value("PATH"), QString("/custom/bin"));
        QCOMPARE(tp.configurePreset, QString("config1"));
        QCOMPARE(tp.inheritConfigureEnvironment, std::optional<bool>(false));
        QCOMPARE(tp.configuration, QString("Debug"));
        QCOMPARE(tp.overwriteConfigurationFile, QStringList() << "CMakeLists.txt");
        QVERIFY(tp.output);
        QCOMPARE(tp.output->shortProgress, true);
        QCOMPARE(tp.output->verbosity, QString("high"));
        QCOMPARE(tp.output->debug, false);
        QCOMPARE(tp.output->outputOnFailure, true);
        QCOMPARE(tp.output->quiet, false);
        QCOMPARE(tp.output->outputLogFile, Utils::FilePath::fromUserInput("log.txt"));
        QCOMPARE(tp.output->outputJUnitFile, Utils::FilePath::fromUserInput("results.xml"));
        QCOMPARE(tp.output->labelSummary, true);
        QCOMPARE(tp.output->subprojectSummary, false);
        QCOMPARE(tp.output->maxPassedTestOutputSize, 1024);
        QCOMPARE(tp.output->maxFailedTestOutputSize, 2048);
        QCOMPARE(tp.output->testOutputTruncation, QString("truncate"));
        QCOMPARE(tp.output->maxTestNameWidth, 80);

        QVERIFY(tp.filter);
        const auto &inc = *tp.filter->include;
        QCOMPARE(inc.name, QString("TestA"));
        QCOMPARE(inc.label, QString("unit"));
        QCOMPARE(inc.useUnion, true);
        const auto &idx = *inc.index;
        QCOMPARE(idx.start, 1);
        QCOMPARE(idx.end, 10);
        QCOMPARE(idx.stride, 2);
        QCOMPARE(idx.specificTests, QList<int>() << 1 << 3 << 5);

        const auto &exc = *tp.filter->exclude;
        QCOMPARE(exc.name, QString("TestB"));
        QCOMPARE(exc.label, QString("integration"));
        const auto &fx = *exc.fixtures;
        QCOMPARE(fx.any, QString("BaseFixture"));
        QCOMPARE(fx.setup, QString("SetupFixture"));
        QCOMPARE(fx.cleanup, QString("CleanupFixture"));

        QVERIFY(tp.execution);
        const auto &exe = *tp.execution;
        QCOMPARE(exe.stopOnFailure, true);
        QCOMPARE(exe.enableFailover, false);
        QCOMPARE(exe.jobs, std::optional<int>(4));
        QCOMPARE(exe.resourceSpecFile, Utils::FilePath::fromUserInput("resources.json"));
        QCOMPARE(exe.testLoad, 2);
        QCOMPARE(exe.showOnly, QString("TestA"));
        QCOMPARE(exe.repeat->mode, QString("fixed"));
        QCOMPARE(exe.repeat->count, 3);
        QCOMPARE(exe.interactiveDebugging, true);
        QCOMPARE(exe.scheduleRandom, false);
        QCOMPARE(exe.timeout, 120);
        QCOMPARE(exe.noTestsAction, QString("skip"));
    }

    void testParseTestPresetsInvalidArray_data()
    {
        QTest::addColumn<QString>("json");
        QTest::addColumn<QString>("errorContains");

        QTest::newRow("not array") << R"({"version":5, "testPresets":"not an array"})"
                                   << "Invalid \"testPresets\" section in file \"invalid.json\".";
        QTest::newRow("array of non objects") << R"({""version":5, testPresets":[1,2,3]})"
                                              << "missing name separator";
    }

    void testParseTestPresetsInvalidArray()
    {
        QFETCH(QString, json);
        QFETCH(QString, errorContains);

        QFile file(QDir::tempPath() + "/invalid.json");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(json.toUtf8());
        file.close();

        PresetsParser parser;
        QString error;
        int errorLine;
        bool ok = parser.parse(Utils::FilePath::fromUserInput(file.fileName()), error, errorLine);
        QVERIFY(!ok);
        QVERIFY(error.contains(errorContains));
    }

    void testTestPresetInheritFrom()
    {
        PresetsDetails::TestPreset parent;
        parent.name = "parent";
        parent.hidden = true;
        parent.inherits = QStringList() << "grandparent";
        parent.condition = CMakeProjectManager::Internal::parseCondition(
            QJsonObject{{"matches", "Darwin"}});
        parent.vendor = QVariantMap{
            {"qt.io/QtCreator/1.0", QVariantMap{{"debugger", "/path/to/dbg"}}}};
        parent.displayName = "Parent Display";
        parent.description = "Parent description";
        parent.environment = Utils::Environment::systemEnvironment();
        parent.environment->set("PARENT_VAR", "value");
        parent.configurePreset = "configP";
        parent.inheritConfigureEnvironment = true;
        parent.configuration = "Release";
        parent.overwriteConfigurationFile = QStringList() << "CMakeLists.txt";
        parent.output = PresetsDetails::Output{
            true,
            "high",
            false,
            true,
            false,
            Utils::FilePath::fromUserInput("log.txt"),
            Utils::FilePath::fromUserInput("out.xml"),
            true,
            false,
            100,
            200,
            "trunc",
            80};
        parent.filter = PresetsDetails::Filter{
            PresetsDetails::Filter::Include{
                QString("TestA"),
                QString("unit"),
                true,
                PresetsDetails::Filter::Include::Index{0, 10, 1, QList<int>{1, 2}, std::nullopt}},
            PresetsDetails::Filter::Exclude{
                QString("TestB"),
                QString("unit"),
                PresetsDetails::Filter::Exclude::Fixtures{
                    "BaseFixture", "SetupFixture", "CleanupFixture"}}};
        parent.execution = PresetsDetails::Execution{
            true,
            false,
            4,
            Utils::FilePath::fromUserInput("spec.json"),
            2,
            "TestA",
            PresetsDetails::Execution::Repeat{"fixed", 3},
            true,
            false,
            120,
            "skip",
            std::nullopt};

        PresetsDetails::TestPreset child;
        child.name = "child";
        // child should inherit all, except: name, hidden, inherits, description and displayName

        child.inheritFrom(parent);

        QCOMPARE(child.name, QString("child")); // name does not inherit
        QCOMPARE(child.hidden, false);          // unchanged
        QCOMPARE(child.inherits, std::nullopt);
        QVERIFY(child.condition);
        QVERIFY(child.vendor);
        QVERIFY(!child.displayName); // not inherited
        QVERIFY(!child.description); // not inherited
        QVERIFY(child.environment);
        QVERIFY(child.configurePreset);
        QCOMPARE(child.inheritConfigureEnvironment, std::optional<bool>(true));
        QVERIFY(child.configuration);
        QVERIFY(child.overwriteConfigurationFile);
        QVERIFY(child.output);
        QVERIFY(child.filter);
        QVERIFY(child.execution);
    }

    void testTestPresetInheritFromPartial()
    {
        PresetsDetails::TestPreset parent;
        parent.environment = Utils::Environment();
        parent.environment->set("VAR1", "A");

        PresetsDetails::TestPreset child;
        child.environment = Utils::Environment();
        child.environment->set("VAR2", "B");

        child.inheritFrom(parent);

        // Environment should be merged: VAR2 remains B, VAR1 added
        QVERIFY(child.environment);
        QCOMPARE(child.environment->value("VAR1"), QString("A"));
        QCOMPARE(child.environment->value("VAR2"), QString("B"));
    }

    void testExpandMacrosTestPreset()
    {
        PresetsDetails::TestPreset preset;
        preset.name = "preset1";
        preset.fileDir = Utils::FilePath::fromUserInput("/tmp/project");
        preset.environment = Utils::Environment();
        preset.environment->set("VAR", "VALUE");

        Utils::Environment env = *preset.environment;

        // Test simple variable expansion
        QString val1 = "$env{VAR}";
        expand(preset, env, FilePath::fromString("/tmp/project"), val1);
        QCOMPARE(val1, QString("VALUE"));

        // Test sourceDir macro
        QString val2 = "${sourceDir}";
        expand(preset, env, FilePath::fromString("/tmp/project"), val2);
        QCOMPARE(val2, QString("/tmp/project"));

        // Test presetName macro
        QString val3 = "Hello ${presetName}";
        expand(preset, env, FilePath::fromString("/tmp/project"), val3);
        QCOMPARE(val3, QString("Hello preset1"));

        // Test hostSystemName macro
        QString val4 = "${hostSystemName}";
        expand(preset, env, FilePath::fromString("/tmp/project"), val4);
        // hostSystemName should be a non-empty string
        QVERIFY(!val4.isEmpty());

        // Test pathListSep macro
        QString val5 = "a${pathListSep}b";
        expand(preset, env, FilePath::fromString("/tmp/project"), val5);
        if (HostOsInfo::isWindowsHost())
            QCOMPARE(val5, QString("a;b"));
        else
            QCOMPARE(val5, QString("a:b"));
    }

    void testPresetToCTestArgs()
    {
        PresetsDetails::TestPreset p;
        PresetsDetails::Output output;
        output.shortProgress = true;
        output.verbosity = "debug";
        output.outputOnFailure = true;
        output.outputLogFile = Utils::FilePath::fromString("/tmp/log.txt");
        output.outputJUnitFile = Utils::FilePath::fromString("/tmp/junit.xml");
        output.labelSummary = false;
        output.subprojectSummary = false;
        output.maxPassedTestOutputSize = 1024;
        output.maxFailedTestOutputSize = 2048;
        output.testOutputTruncation = "full";
        output.maxTestNameWidth = 80;
        p.output = output;

        PresetsDetails::Filter filter;
        PresetsDetails::Filter::Include include;
        include = PresetsDetails::Filter::Include{};
        include.name = ".*Foo.*";
        include.label = "fast";
        include.useUnion = true;
        include.index = PresetsDetails::Filter::Include::Index{
            1, 10, 2, QList<int>{3, 5, 7}, std::nullopt
        };
        filter.include = include;
        p.filter = filter;

        PresetsDetails::Execution execution;
        execution.jobs = 4;
        execution.showOnly = "human";
        execution.repeat = PresetsDetails::Execution::Repeat{"count", 3};
        execution.timeout = 30;
        execution.noTestsAction = "error";
        p.execution = execution;

        p.configuration = "Debug";

        const auto args = presetToCTestArgs(p, ctestPassthroughArgumentsVersion());
        qDebug() << args;

        QVERIFY(args.contains("--progress"));
        QVERIFY(args.contains("--debug"));
        QVERIFY(args.contains("--output-on-failure"));
        QVERIFY(args.contains("--output-log"));
        QVERIFY(args.contains("--output-junit"));
        QVERIFY(args.contains("--no-label-summary"));
        QVERIFY(args.contains("--no-subproject-summary"));
        QVERIFY(args.contains("--test-output-size-passed"));
        QVERIFY(args.contains("--test-output-size-failed"));
        QVERIFY(args.contains("--test-output-truncation"));
        QVERIFY(args.contains("--max-width"));
        QVERIFY(args.contains("--tests-regex"));
        QVERIFY(args.contains("--label-regex"));
        QVERIFY(args.contains("--union"));
        QVERIFY(args.contains("--tests-information"));
        QCOMPARE(args.at(args.indexOf("--tests-information") + 1), QString("1,10,2,3,5,7"));
        QVERIFY(args.contains("--parallel"));
        QVERIFY(args.contains("--show-only=human"));
        QVERIFY(args.contains("--repeat"));
        QVERIFY(args.contains("count:3"));
        QVERIFY(args.contains("--timeout"));
        QVERIFY(args.contains("--build-config"));
    }

    void testParseRunSettings()
    {
        const QByteArray json = R"({
            "version": 3,
            "configurePresets": [
                {
                    "name": "default",
                    "vendor": {
                        "qt.io/QtCreator/1.0": {
                            "runSettings": [
                                {
                                    "target": "myApp",
                                    "displayName": "My App",
                                    "executable": "/bin/myApp",
                                    "arguments": "--verbose",
                                    "workingDirectory": "${sourceDir}",
                                    "useTerminal": true,
                                    "useLibraryPaths": false,
                                    "useDyldSuffix": true,
                                    "useVncDisplay": true,
                                    "enableCategoriesFilter": true,
                                    "x11Forwarding": ":0.0",
                                    "runAs": "root",
                                    "environment": { "MY_VAR": "my_value" },
                                    "active": true
                                },
                                { "target": "myApp" }
                            ]
                        }
                    }
                }
            ]
        })";

        const FilePath file = writeJson("runsettings.json", json);
        PresetsParser parser;
        QString error;
        int errorLine = 0;
        QVERIFY(parser.parse(file, error, errorLine));

        const QList<PresetsDetails::ConfigurePreset> &presets
            = parser.presetsData().configurePresets;
        QCOMPARE(presets.size(), 1);

        const QList<PresetsDetails::RunSettings> &runSettings = presets.at(0).runSettings;
        QCOMPARE(runSettings.size(), 2);

        const PresetsDetails::RunSettings &first = runSettings.at(0);
        QCOMPARE(first.target, QString("myApp"));
        QCOMPARE(first.displayName, std::optional<QString>("My App"));
        QCOMPARE(first.executable, std::optional<QString>("/bin/myApp"));
        QCOMPARE(first.arguments, std::optional<QString>("--verbose"));
        QCOMPARE(first.workingDirectory, std::optional<QString>("${sourceDir}"));
        QCOMPARE(first.useTerminal, std::optional<bool>(true));
        QCOMPARE(first.useLibraryPaths, std::optional<bool>(false));
        QCOMPARE(first.useDyldSuffix, std::optional<bool>(true));
        QCOMPARE(first.useVncDisplay, std::optional<bool>(true));
        QCOMPARE(first.enableCategoriesFilter, std::optional<bool>(true));
        QCOMPARE(first.x11Forwarding, std::optional<QString>(":0.0"));
        QCOMPARE(first.runAs, std::optional<QString>("root"));
        QCOMPARE(first.active, std::optional<bool>(true));
        QVERIFY(first.environment);
        QCOMPARE(first.environment->value("MY_VAR"), QString("my_value"));

        // Everything but the target is optional
        const PresetsDetails::RunSettings &second = runSettings.at(1);
        QCOMPARE(second.target, QString("myApp"));
        QVERIFY(!second.displayName);
        QVERIFY(!second.executable);
        QVERIFY(!second.environment);
        QVERIFY(!second.active);
    }

    void testParseRunSettingsInvalid_data()
    {
        QTest::addColumn<QByteArray>("runSettings");

        QTest::newRow("not an array") << QByteArray(R"({ "target": "myApp" })");
        QTest::newRow("not an object") << QByteArray(R"([ "myApp" ])");
        QTest::newRow("no target") << QByteArray(R"([ { "arguments": "--verbose" } ])");
        QTest::newRow("empty target") << QByteArray(R"([ { "target": "" } ])");
        QTest::newRow("environment not an object")
            << QByteArray(R"([ { "target": "myApp", "environment": "MY_VAR=1" } ])");
    }

    void testParseRunSettingsInvalid()
    {
        QFETCH(QByteArray, runSettings);

        const QByteArray json = R"({
            "version": 3,
            "configurePresets": [
                {
                    "name": "default",
                    "vendor": { "qt.io/QtCreator/1.0": { "runSettings": )"
                                + runSettings + R"( } }
                }
            ]
        })";

        const FilePath file = writeJson("runsettings-invalid.json", json);
        PresetsParser parser;
        QString error;
        int errorLine = 0;
        QVERIFY(!parser.parse(file, error, errorLine));
        QVERIFY(!error.isEmpty());
    }

    void testRunSettingsAreInherited()
    {
        PresetsDetails::RunSettings parentSetting;
        parentSetting.target = "fromParent";

        PresetsDetails::ConfigurePreset parent;
        parent.name = "parent";
        parent.runSettings = {parentSetting};

        PresetsDetails::ConfigurePreset child;
        child.name = "child";
        child.inheritFrom(parent);
        QCOMPARE(child.runSettings.size(), 1);
        QCOMPARE(child.runSettings.at(0).target, QString("fromParent"));

        // Own run settings take precedence over the inherited ones
        PresetsDetails::RunSettings ownSetting;
        ownSetting.target = "own";

        PresetsDetails::ConfigurePreset sibling;
        sibling.name = "sibling";
        sibling.runSettings = {ownSetting};
        sibling.inheritFrom(parent);
        QCOMPARE(sibling.runSettings.size(), 1);
        QCOMPARE(sibling.runSettings.at(0).target, QString("own"));
    }

    void testParseNullValues()
    {
        const QByteArray json = R"({
            "version": 3,
            "configurePresets": [
                {
                    "name": "default",
                    "environment": { "SET": "value", "UNSET": null },
                    "cacheVariables": { "SET": "value", "UNSET": null }
                }
            ]
        })";

        const FilePath file = writeJson("nullvalues.json", json);
        PresetsParser parser;
        QString error;
        int errorLine = 0;
        QVERIFY(parser.parse(file, error, errorLine));

        const PresetsDetails::ConfigurePreset &preset = parser.presetsData().configurePresets.at(0);

        // A null value means "not set by this preset", not "set to an empty value"
        QVERIFY(preset.environment);
        QCOMPARE(preset.environment->value("SET"), QString("value"));
        QVERIFY(!preset.environment->hasKey("UNSET"));

        QVERIFY(preset.cacheVariables);
        QCOMPARE(preset.cacheVariables->stringValueOf("SET"), QString("value"));
        QVERIFY(preset.cacheVariables->contains("UNSET"));
        QVERIFY(preset.cacheVariables->value("UNSET").isUnset);
    }

    void testNullCacheVariableShadowsTheInheritedOne()
    {
        PresetsDetails::ConfigurePreset parent;
        parent.name = "parent";
        parent.cacheVariables = CMakeConfig{CMakeConfigItem("VARIABLE", "fromParent")};

        CMakeConfigItem unset;
        unset.key = "VARIABLE";
        unset.isUnset = true;

        PresetsDetails::ConfigurePreset child;
        child.name = "child";
        child.cacheVariables = CMakeConfig{unset};
        child.inheritFrom(parent);

        QVERIFY(child.cacheVariables->value("VARIABLE").isUnset);
    }

    void testNullEnvironmentValueShadowsTheInheritedOne()
    {
        PresetsDetails::ConfigurePreset parent;
        parent.name = "parent";
        parent.environment = Utils::Environment();
        parent.environment->set("VARIABLE", "fromParent");

        PresetsDetails::ConfigurePreset child;
        child.name = "child";
        child.environment = Utils::Environment();
        child.environment->unset("VARIABLE");
        child.inheritFrom(parent);

        QVERIFY(!child.environment->hasKey("VARIABLE"));
    }

    void testNullEnvironmentValueKeepsTheSurroundingOne()
    {
        PresetsDetails::ConfigurePreset preset;
        preset.name = "preset";
        preset.fileDir = FilePath::fromUserInput("/tmp/project");
        preset.environment = Utils::Environment();
        preset.environment->set("KEEP", "set");
        preset.environment->unset("UNSET");

        Utils::Environment env;
        env.set("KEEP", "outer");
        env.set("UNSET", "outer");
        expand(preset, env, FilePath::fromString("/tmp/project"));

        // A null value only means that this preset does not set the variable, so
        // the value of the environment the preset is applied to survives
        QCOMPARE(env.value("KEEP"), QString("set"));
        QCOMPARE(env.value("UNSET"), QString("outer"));

        Utils::EnvironmentItems items;
        expand(preset, items, FilePath::fromString("/tmp/project"));
        QCOMPARE(items, Utils::EnvironmentItems{Utils::EnvironmentItem("KEEP", "set")});
    }

    void testParseExecutionJobs()
    {
        const auto jobsOf = [](const QByteArray &value) {
            const QByteArray json = R"({
                "version": 11,
                "testPresets": [
                    { "name": "default", "execution": { "jobs": )" + value + R"( } }
                ]
            })";

            PresetsParser parser;
            QString error;
            int errorLine = 0;
            const FilePath file = writeJson("executionjobs.json", json);
            if (!parser.parse(file, error, errorLine))
                return std::optional<std::optional<int>>();
            return parser.presetsData().testPresets.at(0).execution->jobs;
        };

        QCOMPARE(jobsOf("4"), std::optional<int>(4));

        // An empty string omits the job count, which cmake spells as a bare --parallel
        const std::optional<std::optional<int>> omitted = jobsOf("\"\"");
        QVERIFY(omitted);
        QVERIFY(!*omitted);

        // Neither null nor zero nor a negative number nor a non-empty string is a valid
        // job count, and cmake refuses such a file altogether. Zero would reach ctest as
        // --parallel 0, which is an unbounded number of jobs
        QVERIFY(!jobsOf("null"));
        QVERIFY(!jobsOf("0"));
        QVERIFY(!jobsOf("-4"));
        QVERIFY(!jobsOf("\"4\""));
    }

    void testParseEmptyIndexString()
    {
        const QByteArray json = R"({
            "version": 11,
            "testPresets": [
                { "name": "default", "filter": { "include": { "index": "" } } }
            ]
        })";

        PresetsParser parser;
        QString error;
        int errorLine = 0;
        QVERIFY(parser.parse(writeJson("emptyindex.json", json), error, errorLine));

        const PresetsDetails::TestPreset &preset = parser.presetsData().testPresets.at(0);
        QVERIFY(preset.filter->include);

        // An empty file name is no index filter, ctest rejects --tests-information ""
        QVERIFY(!preset.filter->include->index);
        QVERIFY(!presetToCTestArgs(preset, ctestPassthroughArgumentsVersion())
                     .contains("--tests-information"));
    }

    void testParseDiagnosticCategories()
    {
        const QByteArray json = R"({
            "version": 12,
            "configurePresets": [
                {
                    "name": "default",
                    "warnings": { "author": false, "policy": true, "unusedCli": false,
                                  "strict": true, "nonTargetDirective": true },
                    "errors": { "deprecated": true, "unusedCli": true, "bogus": true }
                }
            ]
        })";

        const FilePath file = writeJson("diagnostics.json", json);
        PresetsParser parser;
        QString error;
        int errorLine = 0;
        QVERIFY(parser.parse(file, error, errorLine));

        const PresetsDetails::ConfigurePreset &preset = parser.presetsData().configurePresets.at(0);

        QVERIFY(preset.warnings);
        QCOMPARE(preset.warnings->unusedCli, std::optional<bool>(false));
        QCOMPARE(preset.warnings->categories.value("author"), false);
        QCOMPARE(preset.warnings->categories.value("policy"), true);
        QVERIFY(!preset.warnings->categories.contains("unusedCli"));

        // cmake has no such warning category, so it must not reach the command line
        QVERIFY(!preset.warnings->categories.contains("strict"));
        QVERIFY(!preset.warnings->categories.contains("nonTargetDirective"));

        QVERIFY(preset.errors);
        QCOMPARE(preset.errors->deprecated, std::optional<bool>(true));
        QCOMPARE(preset.errors->categories.value("unusedCli"), true);
        QVERIFY(!preset.errors->categories.contains("bogus"));
        QCOMPARE(diagnosticOptionNames().value("unusedCli"), QString("unused-cli"));
    }

    void testConfigurePresetSubObjectsInheritPerField()
    {
        PresetsDetails::ConfigurePreset parent;
        parent.name = "parent";
        parent.warnings = PresetsDetails::Warnings();
        parent.warnings->dev = true;
        parent.warnings->deprecated = false;
        parent.warnings->categories.insert("policy", false);
        parent.architecture = PresetsDetails::ValueStrategyPair();
        parent.architecture->value = "x64";
        parent.architecture->strategy = PresetsDetails::ValueStrategyPair::Strategy::external;

        PresetsDetails::ConfigurePreset child;
        child.name = "child";
        child.warnings = PresetsDetails::Warnings();
        child.warnings->dev = false;
        child.warnings->categories.insert("author", true);
        child.architecture = PresetsDetails::ValueStrategyPair();
        child.architecture->value = "Win32";

        child.inheritFrom(parent);

        // The fields the child sets win, the ones it does not set are inherited
        QCOMPARE(child.warnings->dev, std::optional<bool>(false));
        QCOMPARE(child.warnings->deprecated, std::optional<bool>(false));
        QCOMPARE(child.warnings->categories.value("author"), true);
        QVERIFY(child.warnings->categories.contains("policy"));
        QCOMPARE(child.warnings->categories.value("policy"), false);

        QCOMPARE(child.architecture->value, std::optional<QString>("Win32"));
        QVERIFY(child.architecture->strategy);
        QVERIFY(*child.architecture->strategy
                == PresetsDetails::ValueStrategyPair::Strategy::external);
    }

    void testTestPresetSubObjectsInheritPerField()
    {
        PresetsDetails::TestPreset parent;
        parent.name = "parent";
        parent.output = PresetsDetails::Output();
        parent.output->quiet = true;
        parent.output->verbosity = "verbose";
        parent.execution = PresetsDetails::Execution();
        parent.execution->timeout = 120;
        parent.execution->stopOnFailure = true;
        parent.filter = PresetsDetails::Filter();
        parent.filter->include = PresetsDetails::Filter::Include();
        parent.filter->include->label = "fast";
        parent.filter->exclude = PresetsDetails::Filter::Exclude();
        parent.filter->exclude->name = "excluded";

        PresetsDetails::TestPreset child;
        child.name = "child";
        child.output = PresetsDetails::Output();
        child.output->quiet = false;
        child.execution = PresetsDetails::Execution();
        child.execution->timeout = 60;
        child.filter = PresetsDetails::Filter();
        child.filter->include = PresetsDetails::Filter::Include();
        child.filter->include->name = "included";

        child.inheritFrom(parent);

        QCOMPARE(child.output->quiet, std::optional<bool>(false));
        QCOMPARE(child.output->verbosity, std::optional<QString>("verbose"));
        QCOMPARE(child.execution->timeout, std::optional<int>(60));
        QCOMPARE(child.execution->stopOnFailure, std::optional<bool>(true));
        QCOMPARE(child.filter->include->name, std::optional<QString>("included"));
        QCOMPARE(child.filter->include->label, std::optional<QString>("fast"));
        QVERIFY(child.filter->exclude);
        QCOMPARE(child.filter->exclude->name, std::optional<QString>("excluded"));
    }

    void testInheritConfigureEnvironmentIsNotOverwritten()
    {
        PresetsDetails::BuildPreset parent;
        parent.name = "parent";

        // The parent leaves it unset, which means true, and must not overwrite the child's false
        PresetsDetails::BuildPreset child;
        child.name = "child";
        child.inheritConfigureEnvironment = false;
        child.inheritFrom(parent);
        QCOMPARE(child.inheritConfigureEnvironment, std::optional<bool>(false));

        PresetsDetails::BuildPreset optingOutParent;
        optingOutParent.name = "optingOutParent";
        optingOutParent.inheritConfigureEnvironment = false;

        PresetsDetails::BuildPreset heir;
        heir.name = "heir";
        heir.inheritFrom(optingOutParent);
        QCOMPARE(heir.inheritConfigureEnvironment, std::optional<bool>(false));
    }

    void testExpandDollarMacro()
    {
        PresetsDetails::TestPreset preset;
        preset.name = "preset1";
        preset.configurePreset = "configure1";
        preset.generator = "Ninja";
        preset.fileDir = FilePath::fromUserInput("/tmp/project");

        const Environment env;
        const FilePath sourceDir = FilePath::fromString("/tmp/project");

        // The dollar sign that ${dollar} expands to does not start another macro
        QString value = "${dollar}{sourceDir}";
        expand(preset, env, sourceDir, value);
        QCOMPARE(value, QString("${sourceDir}"));

        value = "${configurePresetName} ${generator} ${presetName}";
        expand(preset, env, sourceDir, value);
        QCOMPARE(value, QString("configure1 Ninja preset1"));
    }

    void testPresetToCTestArgsIndexFile()
    {
        PresetsDetails::TestPreset preset;
        PresetsDetails::Filter filter;
        filter.include = PresetsDetails::Filter::Include{};
        filter.include->index = PresetsDetails::Filter::Include::Index{};
        filter.include->index->indexFile = "/tmp/tests.txt";
        preset.filter = filter;

        const QStringList args = presetToCTestArgs(preset, ctestPassthroughArgumentsVersion());
        QVERIFY(args.indexOf("--tests-information") >= 0);
        QCOMPARE(args.at(args.indexOf("--tests-information") + 1), QString("/tmp/tests.txt"));
    }

    void testPresetToCTestArgsProcessorCountJobs()
    {
        PresetsDetails::TestPreset preset;
        PresetsDetails::Execution execution;
        // "jobs": "" means as many jobs as there are processors
        execution.jobs = std::optional<int>();
        execution.timeout = 30;
        preset.execution = execution;

        const QStringList args = presetToCTestArgs(preset, ctestPassthroughArgumentsVersion());
        const int parallel = args.indexOf("--parallel");
        QVERIFY(parallel >= 0);
        QVERIFY(args.at(parallel + 1).startsWith("--"));
    }

    void testPresetToCTestArgsOverwriteAndPassthrough()
    {
        PresetsDetails::TestPreset preset;
        preset.overwriteConfigurationFile = QStringList() << "Timeout=60";
        PresetsDetails::Execution execution;
        execution.testPassthroughArguments = QStringList() << "--gtest_shuffle" << "-v";
        preset.execution = execution;

        const QStringList args = presetToCTestArgs(preset, ctestPassthroughArgumentsVersion());
        const int overwrite = args.indexOf("--overwrite");
        QVERIFY(overwrite >= 0);
        QCOMPARE(args.at(overwrite + 1), QString("Timeout=60"));

        // Everything after the separator is passed on to the tests, so it has to come last
        QCOMPARE(args.mid(args.indexOf("--")),
                 QStringList() << "--" << "--gtest_shuffle" << "-v");
    }

    void testExpandTestPreset()
    {
        const FilePath sourceDir = FilePath::fromString("/tmp/project");

        PresetsDetails::TestPreset preset;
        preset.name = "preset1";
        preset.configurePreset = "configure1";
        preset.fileDir = sourceDir;
        preset.environment = Environment();
        preset.environment->set("SUITE", "fast");

        PresetsDetails::Output output;
        output.outputLogFile = FilePath::fromUserInput("${sourceDir}/ctest.log");
        preset.output = output;

        PresetsDetails::Filter filter;
        filter.include = PresetsDetails::Filter::Include{};
        filter.include->label = "$env{SUITE}";
        filter.exclude = PresetsDetails::Filter::Exclude{};
        filter.exclude->name = "${presetName}";
        preset.filter = filter;

        PresetsDetails::Execution execution;
        execution.testPassthroughArguments = QStringList() << "--log=${sourceDirName}";
        preset.execution = execution;

        preset.overwriteConfigurationFile = QStringList() << "Timeout=${dollar}60";

        expandTestPreset(preset, Environment(), sourceDir);

        const QStringList args = presetToCTestArgs(preset, ctestPassthroughArgumentsVersion());
        QCOMPARE(args.at(args.indexOf("--output-log") + 1), QString("/tmp/project/ctest.log"));
        QCOMPARE(args.at(args.indexOf("--label-regex") + 1), QString("fast"));
        QCOMPARE(args.at(args.indexOf("--exclude-regex") + 1), QString("preset1"));
        QCOMPARE(args.at(args.indexOf("--overwrite") + 1), QString("Timeout=$60"));
        QCOMPARE(args.last(), QString("--log=project"));
    }

    void testExpandTestPresetEnvironment()
    {
        const FilePath sourceDir = FilePath::fromString("/tmp/project");

        PresetsDetails::TestPreset preset;
        preset.name = "preset1";
        preset.fileDir = sourceDir;
        preset.environment = Environment();
        preset.environment->set("DATA_DIR", "${sourceDir}/data");
        preset.environment->set("SUITE", "${presetName}");
        preset.environment->set("FROM_OUTER", "$penv{OUTER_VARIABLE}");
        preset.environment->unset("UNSET");

        Environment outer;
        outer.set("OUTER_VARIABLE", "fromOuter");
        outer.set("UNSET", "fromOuter");

        expandTestPreset(preset, outer, sourceDir);

        // The environment of a test preset reaches the test process, so it needs the same
        // macro expansion as the fields that end up on the ctest command line
        QCOMPARE(preset.environment->value("DATA_DIR"), QString("/tmp/project/data"));
        QCOMPARE(preset.environment->value("SUITE"), QString("preset1"));
        QCOMPARE(preset.environment->value("FROM_OUTER"), QString("fromOuter"));

        // A null value survives the expansion as a null value, which means that the preset
        // does not set the variable rather than that it removes it
        QCOMPARE(preset.environment->appliedToEnvironment(outer).value("UNSET"),
                 QString("fromOuter"));
    }

    void testPresetToCTestArgsDebugIsPassedOnce()
    {
        PresetsDetails::TestPreset preset;
        PresetsDetails::Output output;
        output.verbosity = "debug";
        output.debug = true;
        preset.output = output;

        QCOMPARE(presetToCTestArgs(preset, ctestPassthroughArgumentsVersion()).count("--debug"),
                 qsizetype(1));
    }

    void testWithoutUnsetVariablesKeepsTheSurroundingOne()
    {
        Environment presetEnvironment;
        presetEnvironment.set("KEEP", "set");
        presetEnvironment.unset("UNSET");

        Environment outer;
        outer.set("KEEP", "outer");
        outer.set("UNSET", "outer");

        // The preset environment on its own removes the variable from the one it is applied to
        QVERIFY(!presetEnvironment.appliedToEnvironment(outer).hasKey("UNSET"));

        const Environment applied = withoutUnsetVariables(presetEnvironment)
                                        .appliedToEnvironment(outer);

        // A null value only means that the preset does not set the variable
        QCOMPARE(applied.value("KEEP"), QString("set"));
        QCOMPARE(applied.value("UNSET"), QString("outer"));
    }

    void testToolchainFileIgnoresAnEmptyBuildDirectory()
    {
        QTemporaryDir sourceDirectory;
        QVERIFY(sourceDirectory.isValid());
        QTemporaryDir workingDirectory;
        QVERIFY(workingDirectory.isValid());

        const FilePath source = FilePath::fromString(sourceDirectory.path());
        QVERIFY(source.pathAppended("tc.cmake").writeFileContents({}));

        // A toolchain file of the same name next to the running process
        QVERIFY(FilePath::fromString(workingDirectory.path())
                    .pathAppended("tc.cmake")
                    .writeFileContents({}));

        PresetsDetails::ConfigurePreset preset;
        preset.name = "preset";
        preset.fileDir = source;
        preset.toolchainFile = "tc.cmake";

        const QString previousDirectory = QDir::currentPath();
        QVERIFY(QDir::setCurrent(workingDirectory.path()));

        // A preset without a binaryDir has no build directory to resolve against
        Environment env;
        updateToolchainFile(preset, env, source, FilePath());

        QVERIFY(QDir::setCurrent(previousDirectory));

        QCOMPARE(preset.cacheVariables->stringValueOf("CMAKE_TOOLCHAIN_FILE"),
                 source.pathAppended("tc.cmake").path());
    }

    void testToolchainFileExpandsDollarOnlyOnce()
    {
        const FilePath sourceDir = FilePath::fromString("/tmp/project");

        PresetsDetails::ConfigurePreset preset;
        preset.name = "preset";
        preset.fileDir = sourceDir;
        preset.toolchainFile = "${dollar}{sourceDir}/tc.cmake";

        Environment env;
        updateToolchainFile(preset, env, sourceDir, FilePath());
        updateCacheVariables(preset, env, sourceDir);

        // The dollar sign that ${dollar} produced must not start another macro
        QCOMPARE(preset.cacheVariables->stringValueOf("CMAKE_TOOLCHAIN_FILE"),
                 QString("${sourceDir}/tc.cmake"));
    }

    void testPresetToCTestArgsIndexWithSpecificTestsOnly()
    {
        PresetsDetails::TestPreset preset;
        PresetsDetails::Filter filter;
        filter.include = PresetsDetails::Filter::Include{};
        filter.include->index = PresetsDetails::Filter::Include::Index{};
        filter.include->index->specificTests = QList<int>{1, 3};
        preset.filter = filter;

        const QStringList args = presetToCTestArgs(preset, ctestPassthroughArgumentsVersion());

        // An empty leading field makes ctest drop the whole filter and run every test,
        // so ",,,1,3" has to be "0,0,0,1,3"
        QCOMPARE(args.at(args.indexOf("--tests-information") + 1), QString("0,0,0,1,3"));
    }

    void testPassthroughArgumentsNeedANewEnoughCTest()
    {
        PresetsDetails::TestPreset preset;
        PresetsDetails::Execution execution;
        execution.testPassthroughArguments = QStringList{"--gtest_shuffle"};
        preset.execution = execution;

        // A ctest without the separator answers it with "Unknown argument: --" and runs
        // no test at all, so the field is dropped instead
        const QStringList older = presetToCTestArgs(preset, QVersionNumber(4, 3));
        QVERIFY(!older.contains("--"));
        QVERIFY(!older.contains("--gtest_shuffle"));

        const QStringList newer = presetToCTestArgs(preset, ctestPassthroughArgumentsVersion());
        QCOMPARE(newer.mid(newer.indexOf("--")), QStringList() << "--" << "--gtest_shuffle");
    }

    void testCyclicInheritanceIsReported()
    {
        // Two presets that inherit each other used to recurse until the stack was exhausted
        const PresetsData data = combinedPresets(R"({
            "version": 3,
            "configurePresets": [
                { "name": "a", "inherits": ["b"] },
                { "name": "b", "inherits": ["a"] }
            ]
        })");

        QVERIFY(!data.hasValidPresets);
        QCOMPARE(errorMessages(data).filter("Cyclic inheritance").size(), 2);
    }

    void testDiamondInheritanceIsNotACycle()
    {
        // Two presets inheriting the same base form a diamond, which CMake allows
        PresetsData data = combinedPresets(R"({
            "version": 3,
            "configurePresets": [
                { "name": "base", "generator": "Ninja", "binaryDir": "/base" },
                { "name": "left", "inherits": ["base"], "binaryDir": "/left" },
                { "name": "right", "inherits": ["base"] },
                { "name": "top", "inherits": ["left", "right"] }
            ]
        })");

        QCOMPARE(errorMessages(data), QStringList());
        QVERIFY(data.hasValidPresets);

        const PresetsDetails::ConfigurePreset top = presetNamed(data.configurePresets, "top");
        QCOMPARE(top.generator, std::optional<QString>("Ninja"));

        // The first preset of the "inherits" list wins over the later ones and the base
        QCOMPARE(top.binaryDir, std::optional<QString>("/left"));
    }

    void testAPresetDefinedTwiceInOneFileIsDropped()
    {
        const PresetsData data = combinedPresets(R"({
            "version": 3,
            "configurePresets": [
                { "name": "twice", "generator": "Ninja" },
                { "name": "twice", "generator": "Unix Makefiles" }
            ]
        })");

        QVERIFY(!data.hasValidPresets);
        QCOMPARE(data.configurePresets.size(), 1);
        QCOMPARE(data.configurePresets.at(0).generator, std::optional<QString>("Ninja"));
        QCOMPARE(data.errors.size(), 1);
        QCOMPARE(data.errors.at(0).filePath.fileName(), QString("CMakePresets.json"));
    }

    void testUserPresetsCannotRedefineAPreset()
    {
        const PresetsData data = combinedPresets(R"({
            "version": 3,
            "configurePresets": [ { "name": "shared", "generator": "Ninja" } ]
        })",
                                                 R"({
            "version": 4,
            "configurePresets": [ { "name": "shared", "generator": "Unix Makefiles" } ]
        })");

        // The re-defining preset was reported and then used anyway, which duplicated its kit
        QVERIFY(!data.hasValidPresets);
        QCOMPARE(data.configurePresets.size(), 1);
        QCOMPARE(data.configurePresets.at(0).generator, std::optional<QString>("Ninja"));
        QCOMPARE(data.errors.size(), 1);
        QCOMPARE(data.errors.at(0).filePath.fileName(), QString("CMakeUserPresets.json"));
    }

    void testAConfigurePresetNameThatMatchesNothingIsReported()
    {
        PresetsData data = combinedPresets(R"({
            "version": 3,
            "configurePresets": [ { "name": "configure", "generator": "Ninja" } ],
            "buildPresets": [ { "name": "build", "configurePreset": "typo" } ],
            "testPresets": [ { "name": "test", "configurePreset": "typo" } ]
        })");
        QVERIFY(data.hasValidPresets);

        setupBuildPresets(data);
        setupTestPresets(data);

        // Such a name was accepted in silence, leaving the preset without ${generator}
        // and without the configure environment
        QVERIFY(!data.hasValidPresets);
        QCOMPARE(errorMessages(data).filter("missing a corresponding configure preset").size(), 2);
        QVERIFY(!data.buildPresets.at(0).generator);
        QVERIFY(!data.testPresets.at(0).generator);
    }

    void testTheConfigureEnvironmentIsInheritedUnderneathTheOwnOne()
    {
        PresetsData data = combinedPresets(R"({
            "version": 3,
            "configurePresets": [
                {
                    "name": "configure",
                    "generator": "Ninja",
                    "environment": { "FROM_CONFIGURE": "configure", "SHARED": "configure" }
                }
            ],
            "buildPresets": [
                {
                    "name": "build",
                    "configurePreset": "configure",
                    "environment": { "SHARED": "build" }
                },
                {
                    "name": "optedOut",
                    "configurePreset": "configure",
                    "inheritConfigureEnvironment": false,
                    "environment": { "SHARED": "build" }
                }
            ]
        })");

        setupBuildPresets(data);
        QCOMPARE(errorMessages(data), QStringList());

        const PresetsDetails::BuildPreset build = presetNamed(data.buildPresets, "build");
        QCOMPARE(build.generator, std::optional<QString>("Ninja"));
        QVERIFY(build.environment);
        QCOMPARE(build.environment->value("FROM_CONFIGURE"), QString("configure"));

        // The preset keeps its own environment, the inherited one only fills the gaps
        QCOMPARE(build.environment->value("SHARED"), QString("build"));

        const PresetsDetails::BuildPreset optedOut = presetNamed(data.buildPresets, "optedOut");
        QVERIFY(optedOut.environment);
        QVERIFY(!optedOut.environment->hasKey("FROM_CONFIGURE"));
        QCOMPARE(optedOut.environment->value("SHARED"), QString("build"));
    }

private:
    static FilePath writeJson(const QString &fileName, const QByteArray &contents)
    {
        const FilePath file = FilePath::fromUserInput(QDir::tempPath()).pathAppended(fileName);
        if (!file.writeFileContents(contents))
            return {};
        return file;
    }

    // Parses a CMakePresets.json and an optional CMakeUserPresets.json and resolves their
    // inheritance, the way CMakeProject::readPresets() does.
    static PresetsData combinedPresets(const QByteArray &cmakePresets,
                                       const QByteArray &cmakeUserPresets = {})
    {
        auto parse = [](const QString &fileName, const QByteArray &contents) {
            PresetsData data;
            if (contents.isEmpty())
                return data;

            PresetsParser parser;
            QString error;
            int errorLine = 0;
            if (parser.parse(writeJson(fileName, contents), error, errorLine))
                data = parser.presetsData();
            else
                data.errors.append({error, {}});
            return data;
        };

        PresetsData presets = parse("CMakePresets.json", cmakePresets);
        PresetsData userPresets = parse("CMakeUserPresets.json", cmakeUserPresets);
        return combinePresets(presets, userPresets);
    }

    static QStringList errorMessages(const PresetsData &data)
    {
        return Utils::transform(data.errors,
                                [](const PresetsError &error) -> QString { return error.message; });
    }

    template<typename PresetType>
    static PresetType presetNamed(const QList<PresetType> &presets, const QString &name)
    {
        return Utils::findOrDefault(presets, [&name](const PresetType &preset) {
            return preset.name == name;
        });
    }
};

QTEST_GUILESS_MAIN(TestPresetsTests)
#include "tst_cmake_test_presets.moc"
