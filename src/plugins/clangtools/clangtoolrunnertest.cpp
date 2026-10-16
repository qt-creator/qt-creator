// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolrunnertest.h"

#include "clangtoolrunner.h"
#include "clangtoolssettings.h"

#include <cppeditor/clangdiagnosticconfig.h>

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <QtTaskTree/QTaskTree>

using namespace CppEditor;
using namespace QtTaskTree;
using namespace Utils;

Q_DECLARE_METATYPE(CppEditor::ClangToolType)

namespace ClangTools::Internal {

static const char fakeToolScript[] = R"(#!/bin/sh
for arg in "$@"; do
    case "$arg" in
        -export-fixes=*) fixes="${arg#-export-fixes=}" ;;
    esac
done
[ -n "$FAKE_REPORT" ] && cp "$FAKE_REPORT" "$fixes"
[ -n "$FAKE_STDERR" ] && printf '%s\n' "$FAKE_STDERR" >&2
[ -n "$FAKE_SEGV" ] && kill -SEGV $$
exit ${FAKE_EXIT:-0}
)";

static AnalyzeUnit unitFor(const FilePath &file, ClangToolType tool)
{
    FileInfo info;
    info.file = file;
    return AnalyzeUnit(info, tool);
}

class RunnerFixture
{
public:
    RunnerFixture()
    {
        if (!HostOsInfo::isAnyUnixHost())
            return;
        if (!m_dir.isValid())
            return;
        m_tool = dirPath().pathAppended("fake-tool.sh");
        QFile script(m_tool.toFSPathString());
        if (!script.open(QIODevice::WriteOnly))
            return;
        script.write(fakeToolScript);
        script.close();
        script.setPermissions(script.permissions() | QFileDevice::ExeOwner);

        m_source = dirPath().pathAppended("main.cpp");
        QFile source(m_source.toFSPathString());
        if (!source.open(QIODevice::WriteOnly))
            return;
        source.write("int main()\n{\n    return 0;\n}\n");
        source.close();

        m_report = dirPath().pathAppended("report.yaml");
        QFile report(m_report.toFSPathString());
        if (!report.open(QIODevice::WriteOnly))
            return;
        report.write(QString(
            "---\n"
            "MainSourceFile: '%1'\n"
            "Diagnostics:\n"
            "  - DiagnosticName: bugprone-test\n"
            "    DiagnosticMessage:\n"
            "      Message: 'a finding'\n"
            "      FilePath: '%1'\n"
            "      FileOffset: 4\n"
            "      Replacements: []\n"
            "...\n").arg(m_source.path()).toUtf8());
        report.close();

        ClangToolsSettings *settings = ClangToolsSettings::instance();
        m_oldTidy = settings->clangTidyExecutable();
        m_oldClazy = settings->clazyStandaloneExecutable();
        settings->clangTidyExecutable.setValue(m_tool);
        settings->clazyStandaloneExecutable.setValue(m_tool);
        m_ready = true;
    }

    ~RunnerFixture()
    {
        if (!m_ready)
            return;
        ClangToolsSettings *settings = ClangToolsSettings::instance();
        settings->clangTidyExecutable.setValue(m_oldTidy);
        settings->clazyStandaloneExecutable.setValue(m_oldClazy);
    }

    bool isReady() const { return m_ready; }
    FilePath dirPath() const { return FilePath::fromString(m_dir.path()); }
    FilePath source() const { return m_source; }
    FilePath report() const { return m_report; }

    AnalyzeInputData input(ClangToolType tool = ClangToolType::Tidy) const
    {
        AnalyzeInputData result;
        result.tool = tool;
        result.config.setId("Test.Config");
        result.config.setDisplayName("Test Config");
        result.outputDirPath = dirPath();
        result.environment = Environment::systemEnvironment();
        return result;
    }

    QList<AnalyzeOutputData> run(const AnalyzeInputData &input, DoneWith *result = nullptr) const
    {
        QList<AnalyzeOutputData> outputs;
        const AnalyzeUnits units{unitFor(m_source, input.tool)};
        const DoneWith done = QTaskTree::runBlocking(Group{
            clangToolTask(units, input, {},
                          [&outputs](const AnalyzeOutputData &data) { outputs << data; },
                          dirPath())});
        if (result)
            *result = done;
        return outputs;
    }

private:
    QTemporaryDir m_dir;
    FilePath m_tool;
    FilePath m_source;
    FilePath m_report;
    FilePath m_oldTidy;
    FilePath m_oldClazy;
    bool m_ready = false;
};

void ClangToolRunnerTest::testRun()
{
    QFETCH(QString, exitCode);
    QFETCH(QString, stderrText);
    QFETCH(bool, withReport);
    QFETCH(bool, crash);
    QFETCH(int, expectedOutputs);
    QFETCH(bool, expectedSuccess);
    QFETCH(int, expectedDiagnostics);
    QFETCH(QString, expectedMessage);
    QFETCH(QStringList, expectedUnknownFlags);

    RunnerFixture fixture;
    if (!fixture.isReady())
        QSKIP("This test requires a POSIX shell.");

    AnalyzeInputData input = fixture.input();
    input.environment.set("FAKE_EXIT", exitCode);
    input.environment.set("FAKE_STDERR", stderrText);
    if (withReport)
        input.environment.set("FAKE_REPORT", fixture.report().toFSPathString());
    if (crash)
        input.environment.set("FAKE_SEGV", "1");

    const QList<AnalyzeOutputData> outputs = fixture.run(input);

    QCOMPARE(outputs.size(), expectedOutputs);
    const AnalyzeOutputData &last = outputs.last();
    QCOMPARE(last.success, expectedSuccess);
    QCOMPARE(last.diagnostics.size(), expectedDiagnostics);
    QVERIFY2(last.errorMessage.contains(expectedMessage), qPrintable(last.errorMessage));
    QCOMPARE(last.unknownFlags, expectedUnknownFlags);

    // The run information is reported exactly once per file.
    int withRunInfo = 0;
    for (const AnalyzeOutputData &output : outputs) {
        if (!output.runInfo.isEmpty()) {
            ++withRunInfo;
            QVERIFY(output.runInfo.contains("Command line:"));
            QVERIFY(output.runInfo.contains("Working directory:"));
            QVERIFY(output.runInfo.contains("Checks:"));
        }
    }
    QCOMPARE(withRunInfo, 1);
}

void ClangToolRunnerTest::testRun_data()
{
    QTest::addColumn<QString>("exitCode");
    QTest::addColumn<QString>("stderrText");
    QTest::addColumn<bool>("withReport");
    QTest::addColumn<bool>("crash");
    QTest::addColumn<int>("expectedOutputs");
    QTest::addColumn<bool>("expectedSuccess");
    QTest::addColumn<int>("expectedDiagnostics");
    QTest::addColumn<QString>("expectedMessage");
    QTest::addColumn<QStringList>("expectedUnknownFlags");

    const QString unknownFlag = "error: unknown argument: '-fipa-pta' [clang-diagnostic-error]";
    const QString missingHeader = "main.cpp:1:10: error: 'missing.h' file not found";

    QTest::newRow("success")
        << "0" << QString() << true << false << 1 << true << 1 << QString() << QStringList();
    QTest::newRow("success-with-stderr")
        << "0" << "16 warnings generated." << true << false << 2 << true << 1 << QString()
        << QStringList();
    QTest::newRow("unknown-flag-with-report")
        << "1" << unknownFlag << true << false << 1 << false << 0 << "finished with exit code: 1"
        << QStringList{"-fipa-pta"};
    QTest::newRow("unknown-flag-without-report")
        << "1" << unknownFlag << false << false << 1 << false << 0 << "finished with exit code: 1"
        << QStringList{"-fipa-pta"};
    QTest::newRow("unknown-flag-in-successful-run")
        << "0" << unknownFlag << true << false << 2 << true << 1 << QString()
        << QStringList{"-fipa-pta"};
    QTest::newRow("hard-error-with-report")
        << "1" << missingHeader << true << false << 1 << false << 0 << "finished with exit code: 1"
        << QStringList();
    QTest::newRow("crash")
        << "0" << QString() << false << true << 1 << false << 0 << "crashed" << QStringList();
}

void ClangToolRunnerTest::testWithoutOutputHandler()
{
    RunnerFixture fixture;
    if (!fixture.isReady())
        QSKIP("This test requires a POSIX shell.");

    const AnalyzeUnits units{unitFor(fixture.source(), ClangToolType::Tidy)};
    for (const QString &exitCode : {QString("0"), QString("1")}) {
        AnalyzeInputData input = fixture.input();
        input.environment.set("FAKE_EXIT", exitCode);
        const DoneWith done = QTaskTree::runBlocking(
            Group{clangToolTask(units, input, {}, {}, fixture.dirPath())});
        QCOMPARE(done, DoneWith::Success);
    }
}

void ClangToolRunnerTest::testChecksSource()
{
    QFETCH(ClangToolType, tool);
    QFETCH(bool, withConfigFile);
    QFETCH(bool, preferConfigFile);
    QFETCH(QString, expectedText);

    RunnerFixture fixture;
    if (!fixture.isReady())
        QSKIP("This test requires a POSIX shell.");

    if (!withConfigFile && preferConfigFile && tool == ClangToolType::Tidy
        && !FilePath::fromString(QDir::tempPath())
                .searchHereAndInParents(".clang-tidy", Utils::DirFilterFlag::Files).isEmpty()) {
        QSKIP("A .clang-tidy file exists in a parent directory of the temporary directory.");
    }

    if (withConfigFile) {
        QFile configFile(fixture.dirPath().pathAppended(".clang-tidy").toFSPathString());
        QVERIFY(configFile.open(QIODevice::WriteOnly));
        configFile.write("Checks: 'bugprone-*'\n");
        configFile.close();
    }

    AnalyzeInputData input = fixture.input(tool);
    input.runSettings.preferConfigFile = preferConfigFile;
    input.environment.set("FAKE_REPORT", fixture.report().toFSPathString());

    const QList<AnalyzeOutputData> outputs = fixture.run(input);

    QCOMPARE(outputs.size(), 1);
    QVERIFY2(outputs.constFirst().runInfo.contains(expectedText),
             qPrintable(outputs.constFirst().runInfo));
}

void ClangToolRunnerTest::testChecksSource_data()
{
    QTest::addColumn<ClangToolType>("tool");
    QTest::addColumn<bool>("withConfigFile");
    QTest::addColumn<bool>("preferConfigFile");
    QTest::addColumn<QString>("expectedText");

    QTest::newRow("tidy-config-file")
        << ClangToolType::Tidy << true << true << "nearest .clang-tidy found";
    QTest::newRow("tidy-config-file-not-preferred")
        << ClangToolType::Tidy << true << false << "\"Prefer .clang-tidy file\" is off";
    QTest::newRow("tidy-no-config-file")
        << ClangToolType::Tidy << false << true << "no .clang-tidy found";
    QTest::newRow("tidy-no-config-file-not-preferred")
        << ClangToolType::Tidy << false << false << "\"Prefer .clang-tidy file\" is off";
    QTest::newRow("clazy")
        << ClangToolType::Clazy << false << true << "from the diagnostic configuration";
}

} // namespace ClangTools::Internal
