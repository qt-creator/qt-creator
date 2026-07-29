// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidlogcatcrashparser.h"

#include "androidconstants.h"
#include "androidtr.h"

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>

#include <QRegularExpression>

#ifdef WITH_TESTS
#include <projectexplorer/outputparser_test.h>
#include <QTest>
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace Android::Internal {

class AndroidLogcatCrashParser final : public OutputTaskParser
{
    Result handleLine(const QString &line, OutputFormat format) final;
    Result handleContinuation(const QString &line);
    void flush() final;

    enum class CrashKind { None, Java, Native };
    CrashKind m_kind = CrashKind::None;
    Task m_task;
};

OutputLineParser::Result AndroidLogcatCrashParser::handleLine(
    const QString &line, OutputFormat format)
{
    Q_UNUSED(format)

    if (m_kind != CrashKind::None)
        return handleContinuation(line);

    static const QRegularExpression nativeCrashStart(R"(^\w+/DEBUG\s*:\s*\*{3}(?:\s\*{3}){5,}\s*$)");
    if (nativeCrashStart.match(line).hasMatch()) {
        m_kind = CrashKind::Native;
        m_task = OtherTask(Task::Error, Tr::tr("Native crash detected in Android app."));
        m_task.addToDetails(line);
        return Status::InProgress;
    }

    static const QRegularExpression javaCrashStart(
        R"(^\w+/AndroidRuntime\s*:\s*(FATAL EXCEPTION.*)$)");
    const QRegularExpressionMatch javaMatch = javaCrashStart.match(line);
    if (javaMatch.hasMatch()) {
        m_kind = CrashKind::Java;
        m_task = OtherTask(Task::Error, javaMatch.captured(1).trimmed());
        m_task.addToDetails(line);
        return Status::InProgress;
    }

    static const QRegularExpression lowMemoryKill(
        R"(Kill(?:ing)?\s+'([^']+)'\s*\((\d+)\).*(?:oom_score_adj|to free))");
    const QRegularExpressionMatch lmkMatch = lowMemoryKill.match(line);
    if (lmkMatch.hasMatch()) {
        Task task = OtherTask(
            Task::Error,
            Tr::tr("Android low-memory killer terminated \"%1\" (PID %2).")
                .arg(lmkMatch.captured(1), lmkMatch.captured(2)));
        task.addToDetails(line);
        scheduleTask(task, 1);
        return Status::Done;
    }

    return Status::NotHandled;
}

OutputLineParser::Result AndroidLogcatCrashParser::handleContinuation(const QString &line)
{
    static const QRegularExpression androidRuntimeLine(R"(^\w+/AndroidRuntime\s*:\s*(.*)$)");
    static const QRegularExpression debugLine(R"(^\w+/DEBUG\s*:\s*(.*)$)");

    const QRegularExpression &pattern = m_kind == CrashKind::Java ? androidRuntimeLine : debugLine;
    if (!pattern.match(line).hasMatch()) {
        flush();
        return Status::NotHandled;
    }
    m_task.addToDetails(line);
    return Status::InProgress;
}

void AndroidLogcatCrashParser::flush()
{
    if (m_task.isNull())
        return;

    if (m_kind == CrashKind::Native) {
        static const QRegularExpression signalPattern(R"(signal\s+\d+\s+\([A-Z_]+\))");
        for (const QString &detail : m_task.details()) {
            const QRegularExpressionMatch match = signalPattern.match(detail);
            if (match.hasMatch()) {
                m_task.setSummary(
                    Tr::tr("Native crash detected in Android app (%1).")
                        .arg(match.captured().trimmed()));
                break;
            }
        }
    } else if (m_kind == CrashKind::Java) {
        static const QRegularExpression tagPrefix(R"(^\w+/AndroidRuntime\s*:\s*)");
        static const QRegularExpression processLine(R"(\bProcess:\s*\S+,\s*PID:\s*\d+\b)");
        const QStringList details = m_task.details();
        for (int i = 1; i < details.size(); ++i) {
            if (processLine.match(details.at(i)).hasMatch())
                continue;
            m_task.setSummary(QString(details.at(i)).remove(tagPrefix).trimmed());
            break;
        }
    }

    static const int maxDetailLines = 100;
    if (m_task.details().size() > maxDetailLines) {
        QStringList details = m_task.details().mid(0, maxDetailLines);
        details << QStringLiteral("...");
        m_task.setDetails(details);
    }

    scheduleTask(m_task, m_task.details().count());
    m_task.clear();
    m_kind = CrashKind::None;
}

OutputLineParser *createAndroidLogcatCrashParser()
{
    return new AndroidLogcatCrashParser;
}

void setupAndroidLogcatCrashParser()
{
    addOutputParserFactory([](Target *target) -> OutputLineParser * {
        if (target
            && RunDeviceTypeKitAspect::deviceTypeId(target->kit())
                   == Constants::ANDROID_DEVICE_TYPE) {
            return new AndroidLogcatCrashParser;
        }
        return nullptr;
    });
}

} // namespace Android::Internal

#ifdef WITH_TESTS

namespace Android::Internal {

class AndroidLogcatCrashParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void testParser_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<Tasks>("tasks");
        QTest::addColumn<QStringList>("childStdErrLines");

        const QString javaInput
            = "E/AndroidRuntime: FATAL EXCEPTION: main\n"
              "E/AndroidRuntime: Process: my.company.app, PID: 12345\n"
              "E/AndroidRuntime: java.lang.NullPointerException: boom\n"
              "E/AndroidRuntime: \tat my.company.app.MainActivity.onCreate(MainActivity.java:42)\n";
        Task javaTask = OtherTask(Task::Error, "java.lang.NullPointerException: boom");
        javaTask.setDetails({
            "E/AndroidRuntime: FATAL EXCEPTION: main",
            "E/AndroidRuntime: Process: my.company.app, PID: 12345",
            "E/AndroidRuntime: java.lang.NullPointerException: boom",
            "E/AndroidRuntime: \tat my.company.app.MainActivity.onCreate(MainActivity.java:42)",
        });
        QTest::newRow("java exception") << javaInput << Tasks{javaTask} << QStringList();

        const QString nativeInput
            = "F/DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n"
              "F/DEBUG   : Build fingerprint: 'company/app/app:14/UD1A/123:user/release-keys'\n"
              "F/DEBUG   : pid: 12345, tid: 12345, name: my.company.app  >>> my.company.app <<<\n"
              "F/DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x0\n"
              "F/DEBUG   : backtrace:\n"
              "F/DEBUG   :       #00 pc 0000000000012345  /system/lib64/libc.so\n";
        Task nativeTask = OtherTask(
            Task::Error, "Native crash detected in Android app (signal 11 (SIGSEGV)).");
        nativeTask.setDetails({
            "F/DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
            "F/DEBUG   : Build fingerprint: 'company/app/app:14/UD1A/123:user/release-keys'",
            "F/DEBUG   : pid: 12345, tid: 12345, name: my.company.app  >>> my.company.app <<<",
            "F/DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x0",
            "F/DEBUG   : backtrace:",
            "F/DEBUG   :       #00 pc 0000000000012345  /system/lib64/libc.so",
        });
        QTest::newRow("native crash") << nativeInput << Tasks{nativeTask} << QStringList();

        const QString lmkInput
            = "lowmemorykiller: Kill 'my.company.app' (12392), uid 10150, oom_score_adj 0 "
              "to free 984776kB rss, 0kB swap; reason: low watermark is breached and "
              "thrashing (1135%)\n";
        Task lmkTask = OtherTask(
            Task::Error, "Android low-memory killer terminated \"my.company.app\" (PID 12392).");
        lmkTask.addToDetails(lmkInput.trimmed());
        QTest::newRow("low memory kill") << lmkInput << Tasks{lmkTask} << QStringList();

        const QString noCrashInput = "I/my.company.app: regular application log output\n"
                                     "D/SomeTag: nothing to see here\n";
        QTest::newRow("no crash") << noCrashInput << Tasks{}
                                  << QStringList{
                                         "I/my.company.app: regular application log output",
                                         "D/SomeTag: nothing to see here"};
    }

    void testParser()
    {
        OutputParserTester testbench;
        testbench.setLineParsers({new AndroidLogcatCrashParser});
        QFETCH(QString, input);
        QFETCH(Tasks, tasks);
        QFETCH(QStringList, childStdErrLines);
        testbench.testParsing(input, OutputParserTester::STDERR, tasks, {}, childStdErrLines);
    }
};

QObject *createAndroidLogcatCrashParserTest()
{
    return new AndroidLogcatCrashParserTest;
}

} // namespace Android::Internal

#include "androidlogcatcrashparser.moc"

#endif // WITH_TESTS
