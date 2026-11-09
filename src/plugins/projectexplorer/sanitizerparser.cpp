// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sanitizerparser.h"

#include "ioutputparser.h"
#include "projectexplorerconstants.h"
#include "runcontrol.h"
#include "task.h"

#include <QRegularExpression>

#include <iterator>
#include <numeric>

#ifdef WITH_TESTS
#include <QTest>
#include "outputparser_test.h"
#include "taskhub.h"
#endif

using namespace Utils;

namespace ProjectExplorer::Internal {

// Locations are either source files with line and sometimes column, or binaries with
// a hex offset. Files compiled with a relative path appear with that path.
static const QString &locationPattern()
{
    static const QString pattern = [] {
        const QString driveSpec = "[A-Za-z]:";
        const QString separators = R"(\/\\)";
        // In the surrounding text, a relative path is delimited by these characters.
        const QString delimiters = "\\s'\"()";
        const QString absolutePathPattern
            = QString("(?:%1)?[%2][^:]+").arg(driveSpec, separators);
        const QString relativePathPattern
            = QString("(?<=^|[%1])[^%1:%2][^%1:]*").arg(delimiters, separators);
        const QString filePathPattern
            = QString("(?<file>%1|%2)").arg(absolutePathPattern, relativePathPattern);
        const QString numberSuffixPatternTemplate = R"(:(?<%1>\d+))";
        const QString lineSuffixPattern = numberSuffixPatternTemplate.arg("line");
        const QString columnSuffixPattern = numberSuffixPatternTemplate.arg("column");
        const QString offsetSuffixPattern = R"(\+0x[[:xdigit:]]+)";
        return QString(R"(%1(?:(?:%2(%3)?)|%4))")
            .arg(filePathPattern, lineSuffixPattern, columnSuffixPattern, offsetSuffixPattern);
    }();
    return pattern;
}

enum class SanitizerType { None, Address, UB };

class SanitizerParser final : public OutputTaskParser
{
    Result handleLine(const QString &line, OutputFormat format) final;
    void flush() final;

    Result handleAsanLine(const QString &line);
    Result handleAsanContinuation(const QString &line);
    Result handleUbsanLine(const QString &line);
    LinkSpecs linkSpecsForLocation(const QString &line);
    void addLinkSpecs(const LinkSpecs &linkSpecs);
    void finishTask(int skippedLines);

    Task m_task;
    LinkSpecs m_linkSpecs;
    quint64 m_id = 0;
    SanitizerType m_type = SanitizerType::None;
};

OutputLineParser::Result SanitizerParser::handleLine(const QString &line, OutputFormat format)
{
    if (format != OutputFormat::StdErrFormat) {
        // A UBSan report has no end marker, and the program keeps running.
        if (m_type == SanitizerType::UB)
            finishTask(1);
        return Status::NotHandled;
    }

    if (m_type != SanitizerType::Address) {
        const Result result = handleUbsanLine(line);
        if (result.status != Status::NotHandled)
            return result;
    }
    return handleAsanLine(line);
}

// AddressSanitizer reports are framed by "==<id>==ERROR: " and either a summary or
// "==<id>==ABORTING".
OutputLineParser::Result SanitizerParser::handleAsanLine(const QString &line)
{
    // Non-regex shortcut for the common case.
    if (m_id == 0 && !line.startsWith('='))
        return Status::NotHandled;

    static const QRegularExpression idPattern(R"(^==(?<id>\d+)==ERROR: (?<desc>.*)$)");
    const QRegularExpressionMatch match = idPattern.match(line);
    if (!match.hasMatch())
        return m_id == 0 ? Status::NotHandled : handleAsanContinuation(line);

    QTC_ASSERT(m_id == 0, flush());
    m_id = match.captured("id").toULongLong();
    QTC_ASSERT(m_id != 0, return Status::NotHandled);
    m_type = SanitizerType::Address;
    const QString description = match.captured("desc");
    m_task = Task(Task::Error, description, {}, 0, Constants::TASK_CATEGORY_OTHER);
    m_task.addToDetails(line);
    return Status::InProgress;
}

OutputLineParser::Result SanitizerParser::handleAsanContinuation(const QString &line)
{
    m_task.addToDetails(line);

    if (line == QString("==%1==ABORTING").arg(m_id)) {
        flush();
        return Status::Done;
    }

    LinkSpecs linkSpecs;
    const QString summaryPrefix = "SUMMARY: ";
    if (line.startsWith(summaryPrefix)) {
        static const QRegularExpression summaryPatternWithFile(QString(
                R"(^%1(?<desc>.*?) at %2.*$)").arg(summaryPrefix, locationPattern()));
        const QRegularExpressionMatch summaryMatch = summaryPatternWithFile.match(line);
        if (summaryMatch.hasMatch()) {
            m_task.setSummary(summaryMatch.captured("desc"));
            const FilePath file = absoluteFilePath(FilePath::fromUserInput(
                                                       summaryMatch.captured("file")));
            if (file.isAbsolutePath() && fileExists(file)) {
                m_task.setFile(file);
                m_task.setLine(summaryMatch.captured("line").toInt());
                m_task.setColumn(summaryMatch.captured("column").toInt());
                addLinkSpecForAbsoluteFilePath(
                    linkSpecs, file, m_task.line(), m_task.column(), summaryMatch, "file");
                addLinkSpecs(linkSpecs);
            }
        } else {
            m_task.setSummary(line.mid(summaryPrefix.size()));
        }
        flush();
        return {Status::Done, linkSpecs};
    }
    return {Status::InProgress, linkSpecsForLocation(line)};
}

// UndefinedBehaviorSanitizer reports have no frame: They start with a "runtime error" line,
// which can be followed by notes, indented lines (memory dumps, stack frames) and a summary.
OutputLineParser::Result SanitizerParser::handleUbsanLine(const QString &line)
{
    if (m_type == SanitizerType::UB) {
        if (line.startsWith("SUMMARY: UndefinedBehaviorSanitizer: ")) {
            m_task.addToDetails(line);
            const LinkSpecs linkSpecs = linkSpecsForLocation(line);
            finishTask(0);
            return {Status::Done, linkSpecs};
        }
        // Notes refer to an address or to a source location.
        static const QRegularExpression notePattern(
            R"(^(?:0x[[:xdigit:]]+|(?:[A-Za-z]:)?[^:]+:\d+(?::\d+)?): note: )");
        // An empty line ends the stack frames. Only a summary can follow it.
        const bool afterEmptyLine = m_task.details().last().isEmpty();
        if (!afterEmptyLine
            && (line.isEmpty() || line.startsWith(' ') || notePattern.match(line).hasMatch())) {
            m_task.addToDetails(line);
            return {Status::InProgress, linkSpecsForLocation(line)};
        }
        finishTask(1);
    }

    // Non-regex shortcut for the common case.
    static const QString marker = ": runtime error: ";
    const int markerPos = line.indexOf(marker);
    if (markerPos == -1)
        return Status::NotHandled;

    // The file is given as it was passed to the compiler, so it can be relative.
    static const QRegularExpression startPattern(
        R"(^(?<file>(?:[A-Za-z]:)?[^:]+):(?<line>\d+)(?::(?<column>\d+))?$)");
    const QRegularExpressionMatch match = startPattern.match(line.left(markerPos));
    if (!match.hasMatch())
        return Status::NotHandled;

    m_task = Task(Task::Error, line.mid(markerPos + 2), {}, 0, Constants::TASK_CATEGORY_OTHER);
    m_task.addToDetails(line);
    m_type = SanitizerType::UB;

    LinkSpecs linkSpecs;
    const FilePath file = absoluteFilePath(FilePath::fromUserInput(match.captured("file")));
    if (file.isAbsolutePath() && fileExists(file)) {
        m_task.setFile(file);
        m_task.setLine(match.captured("line").toInt());
        m_task.setColumn(match.captured("column").toInt());
        addLinkSpecForAbsoluteFilePath(
            linkSpecs, file, m_task.line(), m_task.column(), match, "file");
        addLinkSpecs(linkSpecs);
    }
    return {Status::InProgress, linkSpecs};
}

OutputLineParser::LinkSpecs SanitizerParser::linkSpecsForLocation(const QString &line)
{
    static const QRegularExpression filePattern(locationPattern());
    LinkSpecs linkSpecs;
    const QRegularExpressionMatch fileMatch = filePattern.match(line);
    if (fileMatch.hasMatch()) {
        const FilePath file = absoluteFilePath(FilePath::fromUserInput(fileMatch.captured("file")));
        if (file.isAbsolutePath() && fileExists(file)) {
            addLinkSpecForAbsoluteFilePath(linkSpecs, file, fileMatch.captured("line").toInt(),
                                           fileMatch.captured("column").toInt(), fileMatch, "file");
            addLinkSpecs(linkSpecs);
        }
    }
    return linkSpecs;
}

void SanitizerParser::addLinkSpecs(const LinkSpecs &linkSpecs)
{
    LinkSpecs adaptedLinkSpecs = linkSpecs;
    const int offset = std::accumulate(m_task.details().cbegin(), m_task.details().cend() - 1,
            0, [](int total, const QString &line) { return total + line.size() + 1;});
    for (LinkSpec &ls : adaptedLinkSpecs)
        ls.startPos += offset;
    m_linkSpecs << adaptedLinkSpecs;
}

void SanitizerParser::flush()
{
    finishTask(0);
}

void SanitizerParser::finishTask(int skippedLines)
{
    if (m_task.isNull())
        return;

    const int outputLines = m_task.details().count();
    if (m_type == SanitizerType::UB) {
        // A report without a summary line ends with the empty line after its stack frames.
        QStringList details = m_task.details();
        while (!details.isEmpty() && details.last().isEmpty())
            details.removeLast();
        m_task.setDetails(details);
    }

    setDetailsFormat(m_task, m_linkSpecs);
    static const int maxLen = 50;
    if (m_task.details().length() > maxLen) {
        QStringList details = m_task.details();
        auto cutOffIt = std::next(details.begin(), maxLen);
        cutOffIt = details.insert(cutOffIt, "...");
        details.erase(std::next(cutOffIt), std::prev(details.end()));
        m_task.setDetails(details);
    }
    scheduleTask(m_task, outputLines, skippedLines);
    m_task.clear();
    m_linkSpecs.clear();
    m_id = 0;
    m_type = SanitizerType::None;
}

OutputLineParser *createSanitizerOutputParser()
{
    return new SanitizerParser;
}

void setupSanitizerOutputParser()
{
    addOutputParserFactory([](Target *) { return new SanitizerParser; });
}

} // namespace ProjectExplorer::Internal

#ifdef WITH_TESTS

namespace ProjectExplorer::Internal {

class SanitizerParserTest : public QObject
{
    Q_OBJECT

private slots:
    void testParser_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<Tasks>("tasks");
        QTest::addColumn<QStringList>("childStdErrLines");

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
                193, Constants::TASK_CATEGORY_OTHER);
        odrTask.setColumn(16);
        QTest::newRow("odr violation")
                << odrInput
                << QList<Task>{odrTask}
                << odrNonMatchedLines;

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
        const QStringList leakNonMatchedLines{QString()};
        const Task leakTask(Task::Error,
                QString("AddressSanitizer: 19 byte(s) leaked in 1 allocation(s).") + leakInput,
                {}, -1, Constants::TASK_CATEGORY_OTHER);
        QTest::newRow("leak") << leakInput << QList<Task>{leakTask} << leakNonMatchedLines;

        // Real output of GCC 16 for globals compiled with a relative path.
        const QStringList relativeOdrLines{
            "==3761886==ERROR: AddressSanitizer: odr-violation (0x556000ee0060):",
            "  [1] size=16 'shared' odr1.cpp:1:5 in /tmp/asan-real/odr-gcc",
            "  [2] size=16 'shared' odrlib.cpp:1:5 in ./libodr-gcc.so",
            "These globals were registered at these points:",
            "  [1]:",
            "    #0 0x7f3a09a54362 in __asan_register_globals (/usr/lib/libasan.so.8+0x54362) "
            "(BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)",
            "    #1 0x556000edd1b4 in _sub_I_00099_1 (/tmp/asan-real/odr-gcc+0x11b4) "
            "(BuildId: a021c8a62d78819dba6f7bbaeff16ca8df62d0dc)",
            "    #2 0x7f3a09227933 in __libc_start_main (/usr/lib/libc.so.6+0x27933) "
            "(BuildId: d320029ad1a23be2f4bce117fd321cbd23a2014b)",
            "    #3 0x556000edd0a4 in _start (/tmp/asan-real/odr-gcc+0x10a4) "
            "(BuildId: a021c8a62d78819dba6f7bbaeff16ca8df62d0dc)",
            "",
            "  [2]:",
            "    #0 0x7f3a09a54362 in __asan_register_globals (/usr/lib/libasan.so.8+0x54362) "
            "(BuildId: b8a4241051a1621937fdc46e867ba7ecb56d96ea)",
            "    #1 0x7f3a0a2b9174 in _sub_I_00099_1 (libodr-gcc.so+0x1174) "
            "(BuildId: 420b878b0202875063de29dacca65bd89c2d5c4f)",
            "    #2 0x7f3a0a30e1ee  (/lib64/ld-linux-x86-64.so.2+0x61ee) "
            "(BuildId: c609e8ec75de304410d3c69f79d96c7bd4d4fdf8)",
            "    #3 0x7f3a0a30e2cc  (/lib64/ld-linux-x86-64.so.2+0x62cc) "
            "(BuildId: c609e8ec75de304410d3c69f79d96c7bd4d4fdf8)",
            "    #4 0x7f3a0a32983f  (/lib64/ld-linux-x86-64.so.2+0x2183f) "
            "(BuildId: c609e8ec75de304410d3c69f79d96c7bd4d4fdf8)",
            "",
            "==3761886==HINT: if you don't care about these errors you may set "
            "ASAN_OPTIONS=detect_odr_violation=0",
            "SUMMARY: AddressSanitizer: odr-violation: global 'shared' at odr1.cpp:1:5 in "
            "/tmp/asan-real/odr-gcc"};
        const QString relativeOdrFrame = "=================================================================";
        const QString relativeOdrAbort = "==3761886==ABORTING";
        Task relativeOdrTask(Task::Error,
                             "AddressSanitizer: odr-violation: global 'shared'\n"
                                 + relativeOdrLines.join('\n'),
                             FilePath::fromUserInput("/build/odr1.cpp"), 1,
                             Constants::TASK_CATEGORY_OTHER);
        relativeOdrTask.setColumn(5);
        QTest::newRow("odr violation, relative paths")
            << (QStringList{relativeOdrFrame} + relativeOdrLines + QStringList{relativeOdrAbort})
                   .join('\n')
            << QList<Task>{relativeOdrTask} << QStringList{relativeOdrFrame, relativeOdrAbort};

        const QString vptrInput = R"(/qtcreator/src/plugins/projectexplorer/taskhandlers.cpp:111:36: runtime error: member call on address 0x7bddec0d4680 which does not point to an object of type 'ConfigTaskHandler'
0x7bddec0d4680: note: object has invalid vptr
 00 00 00 00  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  00 00 00 00
              ^~~~~~~~~~~~~~~~~~~~~~~
              invalid vptr)";
        Task vptrTask(Task::Error,
                      "runtime error: member call on address 0x7bddec0d4680 which does not point "
                      "to an object of type 'ConfigTaskHandler'\n" + vptrInput,
                      FilePath::fromUserInput(
                          "/qtcreator/src/plugins/projectexplorer/taskhandlers.cpp"),
                      111, Constants::TASK_CATEGORY_OTHER);
        vptrTask.setColumn(36);
        QTest::newRow("ubsan") << vptrInput << QList<Task>{vptrTask} << QStringList();

        // Real output of a program with two errors, built with GCC 16 and Clang 23
        // with -fsanitize=undefined.
        const QString overflowGcc = "ub.cpp:6:16: runtime error: signed integer overflow: "
                                    "2147483647 + 1 cannot be represented in type 'int'";
        const QString boolLine = "ub.cpp:13:12: runtime error: load of value 2, which is not a "
                                 "valid value for type 'bool'";
        const QString overflowSummary = "runtime error: signed integer overflow: 2147483647 + 1 "
                                        "cannot be represented in type 'int'";
        const QString boolSummary = "runtime error: load of value 2, which is not a valid value "
                                    "for type 'bool'";
        const FilePath ubFile = FilePath::fromUserInput("/build/ub.cpp");
        const auto ubTask = [&](const QString &summary, const QStringList &lines, int line,
                                int column) {
            Task task(Task::Error, summary + '\n' + lines.join('\n'), ubFile, line,
                      Constants::TASK_CATEGORY_OTHER);
            task.setColumn(column);
            return task;
        };

        // GCC prints no summary. With stack traces, an empty line ends a report.
        const QStringList overflowGccStack{
            overflowGcc,
            "    #0 0x55aed330f1aa in addOne /tmp/ubsan-real/ub.cpp:6",
            "    #1 0x55aed330f287 in main /tmp/ubsan-real/ub.cpp:19",
            "    #2 0x7f7be4027780  (/usr/lib/libc.so.6+0x27780) "
            "(BuildId: d320029ad1a23be2f4bce117fd321cbd23a2014b)",
            "    #3 0x7f7be40278b8 in __libc_start_main (/usr/lib/libc.so.6+0x278b8) "
            "(BuildId: d320029ad1a23be2f4bce117fd321cbd23a2014b)",
            "    #4 0x55aed330f0a4 in _start (/tmp/ubsan-real/ub-gcc+0x10a4) "
            "(BuildId: 28000ea10d15997777849375263e0225b023c44f)"};
        const QStringList boolGccStack{
            boolLine,
            "    #0 0x55aed330f214 in readBool /tmp/ubsan-real/ub.cpp:13",
            "    #1 0x55aed330f296 in main /tmp/ubsan-real/ub.cpp:20",
            "    #2 0x7f7be4027780  (/usr/lib/libc.so.6+0x27780) "
            "(BuildId: d320029ad1a23be2f4bce117fd321cbd23a2014b)",
            "    #3 0x7f7be40278b8 in __libc_start_main (/usr/lib/libc.so.6+0x278b8) "
            "(BuildId: d320029ad1a23be2f4bce117fd321cbd23a2014b)",
            "    #4 0x55aed330f0a4 in _start (/tmp/ubsan-real/ub-gcc+0x10a4) "
            "(BuildId: 28000ea10d15997777849375263e0225b023c44f)"};
        const QString unrelatedLine = "app: runtime error: not from a sanitizer";
        QTest::newRow("ubsan, gcc, stack traces")
            << (overflowGccStack + QStringList{QString()} + boolGccStack
                + QStringList{QString(), unrelatedLine}).join('\n')
            << QList<Task>{ubTask(overflowSummary, overflowGccStack, 6, 16),
                           ubTask(boolSummary, boolGccStack, 13, 12)}
            << QStringList{unrelatedLine};

        // Clang ends each report with a summary, after the empty line if there is a stack trace.
        const QString overflowClang = "ub.cpp:6:14: runtime error: signed integer overflow: "
                                      "2147483647 + 1 cannot be represented in type 'int'";
        const QString overflowClangSummary
            = "SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior ub.cpp:6:14 ";
        const QString boolClangSummary
            = "SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior ub.cpp:13:12 ";
        const QStringList overflowClangStack{
            overflowClang,
            "    #0 0x55d1309c25ae in addOne(int) /tmp/ubsan-real/ub.cpp:6:14",
            "    #1 0x55d1309c24f5 in main /tmp/ubsan-real/ub.cpp:19:18",
            "    #2 0x7f4fa4c27780 in __libc_start_call_main "
            "/usr/src/debug/glibc/glibc/csu/../sysdeps/nptl/libc_start_call_main.h:59:16",
            "    #3 0x7f4fa4c278b8 in __libc_start_main "
            "/usr/src/debug/glibc/glibc/csu/../csu/libc-start.c:372:3",
            "    #4 0x55d130994114 in _start (/tmp/ubsan-real/ub-clang+0x5114) "
            "(BuildId: be6b7ac75436bce998c4a135c55819dd5162933f)",
            "",
            overflowClangSummary};
        const QStringList boolClangStack{
            boolLine,
            "    #0 0x55d1309c263b in readBool(unsigned char const*) /tmp/ubsan-real/ub.cpp:13:12",
            "    #1 0x55d1309c2501 in main /tmp/ubsan-real/ub.cpp:20:9",
            "    #2 0x7f4fa4c27780 in __libc_start_call_main "
            "/usr/src/debug/glibc/glibc/csu/../sysdeps/nptl/libc_start_call_main.h:59:16",
            "    #3 0x7f4fa4c278b8 in __libc_start_main "
            "/usr/src/debug/glibc/glibc/csu/../csu/libc-start.c:372:3",
            "    #4 0x55d130994114 in _start (/tmp/ubsan-real/ub-clang+0x5114) "
            "(BuildId: be6b7ac75436bce998c4a135c55819dd5162933f)",
            "",
            boolClangSummary};
        QTest::newRow("ubsan, clang, stack traces")
            << (overflowClangStack + boolClangStack).join('\n')
            << QList<Task>{ubTask(overflowSummary, overflowClangStack, 6, 14),
                           ubTask(boolSummary, boolClangStack, 13, 12)}
            << QStringList();

        // Real output of Clang 23 for a source path with a space and notes that refer to it.
        const QString nonnullDir = "/tmp/ubsan-real/My Project/";
        const QStringList nonnullArgLines{
            nonnullDir + "nonnull.cpp:16:31: runtime error: null pointer passed as argument 1, "
                         "which is declared to never be null",
            nonnullDir + "nonnull.cpp:3:16: note: nonnull attribute specified here",
            "SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior " + nonnullDir
                + "nonnull.cpp:16:31 "};
        const QStringList nonnullReturnLines{
            nonnullDir + "nonnull.cpp:10:5: runtime error: null pointer returned from function "
                         "declared to never return null",
            nonnullDir + "nonnull.cpp:8:16: note: returns_nonnull attribute specified here",
            "SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior " + nonnullDir
                + "nonnull.cpp:10:5 "};
        const FilePath nonnullFile = FilePath::fromUserInput(nonnullDir + "nonnull.cpp");
        Task nonnullArgTask(Task::Error,
                            "runtime error: null pointer passed as argument 1, which is declared "
                            "to never be null\n" + nonnullArgLines.join('\n'),
                            nonnullFile, 16, Constants::TASK_CATEGORY_OTHER);
        nonnullArgTask.setColumn(31);
        Task nonnullReturnTask(Task::Error,
                               "runtime error: null pointer returned from function declared to "
                               "never return null\n" + nonnullReturnLines.join('\n'),
                               nonnullFile, 10, Constants::TASK_CATEGORY_OTHER);
        nonnullReturnTask.setColumn(5);
        QTest::newRow("ubsan, clang, notes with a space in the path")
            << (nonnullArgLines + nonnullReturnLines).join('\n')
            << QList<Task>{nonnullArgTask, nonnullReturnTask} << QStringList();

        // Real output of GCC 16 for a report with a note and a memory dump, followed by
        // indented output of the program.
        const QStringList vptrGccLines{
            "vptr2.cpp:12:36: runtime error: member call on address 0x7ffc24810af0 which does "
            "not point to an object of type 'Base'",
            "0x7ffc24810af0: note: object is of type 'Other'",
            " fc 7f 00 00  68 2d a9 f9 37 56 00 00  00 00 00 00 00 00 00 00  01 00 00 00 00 00 00 "
            "00  00 c1 e3 cf",
            "              ^~~~~~~~~~~~~~~~~~~~~~~",
            "              vptr for 'Other'",
            "    #0 0x5637f9a902ed in main /tmp/ubsan-real/vptr2.cpp:12",
            "    #1 0x7f99c0027780  (/usr/lib/libc.so.6+0x27780) "
            "(BuildId: d320029ad1a23be2f4bce117fd321cbd23a2014b)",
            "    #2 0x7f99c00278b8 in __libc_start_main (/usr/lib/libc.so.6+0x278b8) "
            "(BuildId: d320029ad1a23be2f4bce117fd321cbd23a2014b)",
            "    #3 0x5637f9a900d4 in _start (/tmp/ubsan-real/vptr2-gcc+0x10d4) "
            "(BuildId: f507bd4fd3c96a856f34b2d1cffa5cd55cb272fc)"};
        const QString indentedOutput = "    indented program output";
        Task vptrGccTask(Task::Error,
                         "runtime error: member call on address 0x7ffc24810af0 which does not "
                         "point to an object of type 'Base'\n" + vptrGccLines.join('\n'),
                         FilePath::fromUserInput("/build/vptr2.cpp"), 12,
                         Constants::TASK_CATEGORY_OTHER);
        vptrGccTask.setColumn(36);
        QTest::newRow("ubsan, gcc, stack trace, then indented output")
            << (vptrGccLines + QStringList{QString(), indentedOutput}).join('\n')
            << QList<Task>{vptrGccTask} << QStringList{indentedOutput};

        const QStringList overflowClangPlain{overflowClang, overflowClangSummary};
        const QStringList boolClangPlain{boolLine, boolClangSummary};
        QTest::newRow("ubsan, clang, no stack traces")
            << (overflowClangPlain + boolClangPlain).join('\n')
            << QList<Task>{ubTask(overflowSummary, overflowClangPlain, 6, 14),
                           ubTask(boolSummary, boolClangPlain, 13, 12)}
            << QStringList();
    }

    void testUbsanReportFollowedByStdOut()
    {
        Tasks tasks;
        connect(&taskHub(), &TaskHub::taskAdded, this, [&tasks](const Task &t) { tasks << t; });
        OutputParserTester formatter;
        formatter.setLineParsers({new SanitizerParser});
        formatter.appendMessage("ub.cpp:6:16: runtime error: signed integer overflow: "
                                "2147483647 + 1 cannot be represented in type 'int'\n",
                                StdErrFormat);
        formatter.appendMessage("unrelated output -2147483648\n", StdOutFormat);
        formatter.flush();
        taskHub().disconnect(this);
        QCOMPARE(tasks.size(), 1);
        QCOMPARE(tasks.first().summary(),
                 "runtime error: signed integer overflow: 2147483647 + 1 cannot be represented "
                 "in type 'int'");
    }

    void testParser()
    {
        OutputParserTester testbench;
        testbench.setLineParsers({new SanitizerParser});
        testbench.addSearchDir(FilePath::fromUserInput("/build"));
        QFETCH(QString, input);
        QFETCH(Tasks, tasks);
        QFETCH(QStringList, childStdErrLines);
        testbench.testParsing(input, OutputParserTester::STDERR, tasks, {}, childStdErrLines);
    }
};

QObject *createSanitizerOutputParserTest()
{
    return new SanitizerParserTest;
}

} // ProjectExplorer::Internal

#include <sanitizerparser.moc>

#endif // WITH_TESTS
