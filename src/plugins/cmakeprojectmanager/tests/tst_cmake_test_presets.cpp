// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include "../presetsmacros.h"
#include "../presetsparser.h"

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace CMakeProjectManager::Internal::CMakePresets::Macros;

using namespace CMakePresets::Macros;
using namespace Utils;

namespace CMakeProjectManager::Internal {
std::optional<PresetsDetails::Condition> parseCondition(const QJsonValue &jsonValue);
}

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
        QCOMPARE(tp.inheritConfigureEnvironment, true);
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
                        "oputputLogFile": "log.txt",
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
        QCOMPARE(tp.inheritConfigureEnvironment, false);
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
        const auto &inc = tp.filter->include.value();
        QCOMPARE(inc.name, QString("TestA"));
        QCOMPARE(inc.label, QString("unit"));
        QCOMPARE(inc.useUnion, true);
        const auto &idx = inc.index.value();
        QCOMPARE(idx.start, 1);
        QCOMPARE(idx.end, 10);
        QCOMPARE(idx.stride, 2);
        QCOMPARE(idx.specificTests, QList<int>() << 1 << 3 << 5);

        const auto &exc = tp.filter->exclude.value();
        QCOMPARE(exc.name, QString("TestB"));
        QCOMPARE(exc.label, QString("integration"));
        const auto &fx = exc.fixtures.value();
        QCOMPARE(fx.any, QString("BaseFixture"));
        QCOMPARE(fx.setup, QString("SetupFixture"));
        QCOMPARE(fx.cleanup, QString("CleanupFixture"));

        QVERIFY(tp.execution);
        const auto &exe = tp.execution.value();
        QCOMPARE(exe.stopOnFailure, true);
        QCOMPARE(exe.enableFailover, false);
        QCOMPARE(exe.jobs, 4);
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
                PresetsDetails::Filter::Include::Index{0, 10, 1, QList<int>{1, 2}}},
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
            "skip"};

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
        QCOMPARE(child.inheritConfigureEnvironment, true);
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

        Utils::Environment env = preset.environment.value();

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
};

QTEST_GUILESS_MAIN(TestPresetsTests)
#include "tst_cmake_test_presets.moc"
