// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sanitizerparser.h"

#include "projectexplorerconstants.h"

#include <QRegularExpression>

#include <iterator>
#include <numeric>

#ifdef WITH_TESTS
#include <QTest>
#include "outputparser_test.h"
#endif

using namespace Utils;

namespace ProjectExplorer::Internal {

OutputLineParser::Result SanitizerParser::handleLine(const QString &line, OutputFormat format)
{
    if (format != OutputFormat::StdErrFormat)
        return Status::NotHandled;

    // Non-regex shortcut for the common case.
    if (m_id == 0 && !line.startsWith('='))
        return Status::NotHandled;

    static const QRegularExpression idPattern(R"(^==(?<id>\d+)==ERROR: (?<desc>.*)$)");
    const QRegularExpressionMatch match = idPattern.match(line);
    if (!match.hasMatch())
        return m_id == 0 ? Status::NotHandled : handleContinuation(line);

    QTC_ASSERT(m_id == 0, flush());
    m_id = match.captured("id").toULongLong();
    QTC_ASSERT(m_id != 0, return Status::NotHandled);
    const QString description = match.captured("desc");
    m_task = Task(Task::Error, description, {}, 0, Constants::TASK_CATEGORY_SANITIZER);
    m_task.details << line;
    return Status::InProgress;
}

OutputLineParser::Result SanitizerParser::handleContinuation(const QString &line)
{
    m_task.details << line;

    if (line == QString("==%1==ABORTING").arg(m_id)) {
        flush();
        return Status::Done;
    }

    // Locations are either source files with line and sometimes column, or binaries with
    // a hex offset.
    static const QString filePathPattern = R"((?<file>(?:[A-Za-z]:)?[\/\\][^:]+))";
    static const QString numberSuffixPatternTemplate = R"(:(?<%1>\d+))";
    static const QString lineSuffixPattern = numberSuffixPatternTemplate.arg("line");
    static const QString columnSuffixPattern = numberSuffixPatternTemplate.arg("column");
    static const QString offsetSuffixPattern = R"(\+0x[[:xdigit:]]+)";
    static const QString locationPatternString = QString(R"(%1(?:(?:%2(%3)?)|%4))")
            .arg(filePathPattern, lineSuffixPattern, columnSuffixPattern, offsetSuffixPattern);
    static const QRegularExpression filePattern(locationPatternString);

    LinkSpecs linkSpecs;
    const QString summaryPrefix = "SUMMARY: ";
    if (line.startsWith(summaryPrefix)) {
        static const QRegularExpression summaryPatternWithFile(QString(
                R"(^%1(?<desc>.*?) at %2.*$)").arg(summaryPrefix, locationPatternString));
        const QRegularExpressionMatch summaryMatch = summaryPatternWithFile.match(line);
        if (summaryMatch.hasMatch()) {
            m_task.summary = summaryMatch.captured("desc");
            const FilePath file = absoluteFilePath(FilePath::fromUserInput(
                                                       summaryMatch.captured("file")));
            if (fileExists(file)) {
                m_task.file = file;
                m_task.line = summaryMatch.captured("line").toInt();
                m_task.column = summaryMatch.captured("column").toInt();
                addLinkSpecForAbsoluteFilePath(linkSpecs, file, m_task.line, summaryMatch, "file");
                addLinkSpecs(linkSpecs);
            }
        } else {
            m_task.summary = line.mid(summaryPrefix.length());
        }
        flush();
        return {Status::Done, linkSpecs};
    }
    const QRegularExpressionMatch fileMatch = filePattern.match(line);
    if (fileMatch.hasMatch()) {
        const FilePath file = absoluteFilePath(FilePath::fromUserInput(fileMatch.captured("file")));
        if (fileExists(file)) {
            addLinkSpecForAbsoluteFilePath(linkSpecs, file, fileMatch.captured("line").toInt(),
                                           fileMatch, "file");
            addLinkSpecs(linkSpecs);
        }
    }
    return {Status::InProgress, linkSpecs};
}

void SanitizerParser::addLinkSpecs(const LinkSpecs &linkSpecs)
{
    LinkSpecs adaptedLinkSpecs = linkSpecs;
    const int offset = std::accumulate(m_task.details.cbegin(), m_task.details.cend() - 1,
            0, [](int total, const QString &line) { return total + line.length() + 1;});
    for (LinkSpec &ls : adaptedLinkSpecs)
        ls.startPos += offset;
    m_linkSpecs << adaptedLinkSpecs;
}

void SanitizerParser::flush()
{
    if (m_task.isNull())
        return;

    setDetailsFormat(m_task, m_linkSpecs);
    static const int maxLen = 50;
    if (m_task.details.length() > maxLen) {
        auto cutOffIt = std::next(m_task.details.begin(), maxLen);
        cutOffIt = m_task.details.insert(cutOffIt, "...");
        m_task.details.erase(std::next(cutOffIt), std::prev(m_task.details.end()));
    }
    scheduleTask(m_task, m_task.details.count());
    m_task.clear();
    m_linkSpecs.clear();
    m_id = 0;
}

#ifdef WITH_TESTS
class SanitizerParserTest : public QObject
{
    Q_OBJECT

private slots:
    void testParser_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<Tasks >("tasks");
        QTest::addColumn<QString>("childStdErrLines");

        const QString odrInput = R"(=================================================================
==3792966==ERROR: AddressSanitizer: odr-violation (0x55f0cfaeddc0):
  [1] size=16 'lre_id_continue_table_ascii' /sda/home/christian/dev/qbs/master/src/src/shared/quickjs/libregexp.c:193:16
  [2] size=16 'lre_id_continue_table_ascii' /sda/home/christian/dev/qbs/master/src/src/shared/quickjs/libregexp.c:193:16
These globals were registered at these points:
  [1]:
    #0 0x7fc07337a3d9 in __asan_register_globals /usr/src/debug/gcc/libsanitizer/asan/asan_globals.cpp:341
    #1 0x55f0cfa986f2 in _sub_I_00099_1 (/sda/home/christian/dev/qbs/master/qtc_System_with_local_compiler_Debug/install-root/usr/local/bin/tst_language+0x2756f2)
    #2 0x7fc07198943a in __libc_start_main@GLIBC_2.2.5 (/usr/lib/libc.so.6+0x2d43a)

  [2]:
    #0 0x7fc07337a3d9 in __asan_register_globals /usr/src/debug/gcc/libsanitizer/asan/asan_globals.cpp:341
    #1 0x7fc072f456b7 in _sub_I_00099_1 (/sda/home/christian/dev/qbs/master/qtc_System_with_local_compiler_Debug/install-root/usr/local/bin/../lib/libqbscore.so.1.22+0xb926b7)
    #2 0x7fc073d5cedd in call_init (/lib64/ld-linux-x86-64.so.2+0x5edd)

==3792966==HINT: if you don't care about these errors you may set ASAN_OPTIONS=detect_odr_violation=0
SUMMARY: AddressSanitizer: odr-violation: global 'lre_id_continue_table_ascii' at /sda/home/christian/dev/qbs/master/src/src/shared/quickjs/libregexp.c:193:16
==3792966==ABORTING)";
        const QStringList odrNonMatchedLines{
                "=================================================================",
                "==3792966==ABORTING"};
        Task odrTask(Task::Error,
                QString("AddressSanitizer: odr-violation: global 'lre_id_continue_table_ascii'")
                                  + R"(
==3792966==ERROR: AddressSanitizer: odr-violation (0x55f0cfaeddc0):
  [1] size=16 'lre_id_continue_table_ascii' /sda/home/christian/dev/qbs/master/src/src/shared/quickjs/libregexp.c:193:16
  [2] size=16 'lre_id_continue_table_ascii' /sda/home/christian/dev/qbs/master/src/src/shared/quickjs/libregexp.c:193:16
These globals were registered at these points:
  [1]:
    #0 0x7fc07337a3d9 in __asan_register_globals /usr/src/debug/gcc/libsanitizer/asan/asan_globals.cpp:341
    #1 0x55f0cfa986f2 in _sub_I_00099_1 (/sda/home/christian/dev/qbs/master/qtc_System_with_local_compiler_Debug/install-root/usr/local/bin/tst_language+0x2756f2)
    #2 0x7fc07198943a in __libc_start_main@GLIBC_2.2.5 (/usr/lib/libc.so.6+0x2d43a)

  [2]:
    #0 0x7fc07337a3d9 in __asan_register_globals /usr/src/debug/gcc/libsanitizer/asan/asan_globals.cpp:341
    #1 0x7fc072f456b7 in _sub_I_00099_1 (/sda/home/christian/dev/qbs/master/qtc_System_with_local_compiler_Debug/install-root/usr/local/bin/../lib/libqbscore.so.1.22+0xb926b7)
    #2 0x7fc073d5cedd in call_init (/lib64/ld-linux-x86-64.so.2+0x5edd)

==3792966==HINT: if you don't care about these errors you may set ASAN_OPTIONS=detect_odr_violation=0
SUMMARY: AddressSanitizer: odr-violation: global 'lre_id_continue_table_ascii' at /sda/home/christian/dev/qbs/master/src/src/shared/quickjs/libregexp.c:193:16)",
                FilePath::fromUserInput("/sda/home/christian/dev/qbs/master/src/src/shared/quickjs/libregexp.c"),
                193, Constants::TASK_CATEGORY_SANITIZER);
        odrTask.column = 16;
        QTest::newRow("odr violation")
                << odrInput
                << QList<Task>{odrTask}
                << (odrNonMatchedLines.join('\n') + "\n");

        const QString leakInput = R"(
==61167==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 19 byte(s) in 1 object(s) allocated from:
    #0 0x7eff1fd87667 in __interceptor_malloc (/lib64/libasan.so.6+0xb0667)
    #1 0x741c95 in mutt_mem_malloc mutt/memory.c:95
    #2 0x48f089 in get_hostname /home/mutt/work/neo/init.c:343
    #3 0x49259e in mutt_init /home/mutt/work/neo/init.c:929
    #4 0x4a4caa in main /home/mutt/work/neo/main.c:665
    #5 0x7eff1ea1c041 in __libc_start_main ../csu/libc-start.c:308

SUMMARY: AddressSanitizer: 19 byte(s) leaked in 1 allocation(s).)";
        const QString leakNonMatchedLines = "\n";
        const Task leakTask(Task::Error,
                QString("AddressSanitizer: 19 byte(s) leaked in 1 allocation(s).") + leakInput,
                {}, -1, Constants::TASK_CATEGORY_SANITIZER);
        QTest::newRow("leak") << leakInput << QList<Task>{leakTask} << leakNonMatchedLines;
    }

    void testParser()
    {
        OutputParserTester testbench;
        testbench.setLineParsers({new SanitizerParser});
        QFETCH(QString, input);
        QFETCH(Tasks, tasks);
        QFETCH(QString, childStdErrLines);
        testbench.testParsing(input, OutputParserTester::STDERR, tasks, {}, childStdErrLines, {});
    }
};
#endif

std::optional<std::function<QObject *()>> SanitizerParser::testCreator()
{
#ifdef WITH_TESTS
    return []() -> QObject * { return new SanitizerParserTest; };
#else
    return {};
#endif
}

SanitizerOutputFormatterFactory::SanitizerOutputFormatterFactory()
{
    setFormatterCreator([](Target *) -> QList<OutputLineParser *> {return {new SanitizerParser}; });
}

} // namespace ProjectExplorer::Internal

#ifdef WITH_TESTS
#include <sanitizerparser.moc>
#endif
