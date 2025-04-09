// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ldparser.h"
#include "lldparser.h"

#include <utils/algorithm.h>

#include <QRegularExpression>

#ifdef WITH_TESTS
#include "outputparser_test.h"
#include <QTest>
#endif // WITH_TESTS

using namespace Utils;

namespace ProjectExplorer::Internal {

LdParser::LdParser()
{
    setObjectName(QLatin1String("LdParser"));
}

OutputLineParser::Result LdParser::handleLine(const QString &line, OutputFormat type)
{
    if (type != StdErrFormat)
        return Status::NotHandled;

    QString lne = rightTrimmed(line);
    if (lne.startsWith(QLatin1String("TeamBuilder "))
            || lne.startsWith(QLatin1String("distcc["))
            || lne.contains(QLatin1String("ar: creating "))
            || lne.contains("lld:")
            || lne.contains(">>>")) {
        return Status::NotHandled;
    }

    // ld on macOS
    if (lne.startsWith("Undefined symbols for architecture")
        && getStatus(lne) == Status::InProgress) {
        createOrAmendTask(Task::Error, lne, line);
        return Status::InProgress;
    }

    if (!currentTask().isNull() && isContinuation(line)) {
        static const QRegularExpression locRegExp("    (?<symbol>\\S+) in (?<file>\\S+)");
        const QRegularExpressionMatch match = locRegExp.match(lne);
        LinkSpecs linkSpecs;
        FilePath filePath;
        bool handle = false;
        if (match.hasMatch()) {
            handle = true;
            filePath = absoluteFilePath(FilePath::fromString(match.captured("file")));
            addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, -1, -1, match, "file");
            currentTask().setFile(filePath);
        } else {
            handle = !lne.isEmpty() && lne.at(0).isSpace();
        }
        if (handle) {
            createOrAmendTask(Task::Unknown, {}, line, true, filePath);
            return {Status::InProgress, linkSpecs};
        }
    }

    if (lne.startsWith("collect2:") || lne.startsWith("collect2.exe:")) {
        createOrAmendTask(Task::Error, lne, line);
        return getStatus(lne);
    }

    if (const auto result = checkRanlib(lne, line))
        return *result;
    if (const auto result = checkMainRegex(lne, line))
        return *result;

    return Status::NotHandled;
}

bool LdParser::isContinuation(const QString &line) const
{
    return currentTask().details.last().endsWith(':') || (!line.isEmpty() && line.at(0).isSpace());
}

std::optional<OutputLineParser::Result> LdParser::checkRanlib(
    const QString &trimmedLine, const QString &originalLine)
{
    static const QRegularExpression regex("ranlib(.exe)?: (file: (.*) has no symbols)$");
    const QRegularExpressionMatch match = regex.match(trimmedLine);
    if (match.hasMatch()) {
        createOrAmendTask(Task::Warning, match.captured(2), originalLine);
        return getStatus(trimmedLine);
    }
    return {};
}

std::optional<OutputLineParser::Result> LdParser::checkMainRegex(
    const QString &trimmedLine, const QString &originalLine)
{
    static const auto makePattern = [] {
        const QString command
            = R"re((?<cmd>(?:.*[\\\/])?(?:[a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(?:ld|gold)(?:-[0-9\.]+)?(?:\.exe)?: ))re";
        const QString driveSpec = "(?:[A-Za-z]:)?";
        const QString file = QString(R"re(%1[^:]+\.[^:]+)re").arg(driveSpec);
        const QString elfSegmentAndOffset = R"re((?:\(\..*[+-]0x[a-fA-F0-9]+\)))re";
        const QString objFile = QString(R"re((?:(?<obj>%1[^:]+\.(?:o|a|dll|dylib|so))(?::%2)?:))re")
                                    .arg(driveSpec, elfSegmentAndOffset);
        const QString lineNumber = R"re((?<line>[0-9]+))re";
        const QString identifier = "(?:[A-Za-z_][A-Za-z_0-9]*)";
        const QString scopedIdentifier = QString("(?:%1(?:::%1)*)").arg(identifier);
        const QString position
            = QString("(?:%1(?::%2)?|%2|%3)").arg(lineNumber, elfSegmentAndOffset, scopedIdentifier);
        const QString srcFile = QString("(?:(?<src>%1)(?::%2)?: )").arg(file, position);
        const QString description = "(?<desc>.+)";

        return QString(R"re(^%1?%2?%3? ?%4$)re").arg(command, objFile, srcFile, description);
    };
    static const QRegularExpression regex(makePattern());

    const QRegularExpressionMatch match = regex.match(trimmedLine);
    if (!match.hasMatch())
        return {};

    QString description = match.captured("desc").trimmed();
    static const QStringList keywords{
        "File format not recognized",
        "undefined reference",
        "multiple definition",
        "first defined here",
        "cannot find -l",
        "feupdateenv is not implemented and will always fail", // yes, this is quite special ...
    };
    const auto descriptionContainsKeyword = [&description](const QString &keyword) {
        return description.contains(keyword);
    };
    const bool hasKeyword = Utils::anyOf(keywords, descriptionContainsKeyword);

    // Note that a source file with no position information will be detected as an object file
    // if no object file also occurs on the same line. This is harmless.
    LinkSpecs linkSpecs;
    FilePath objFilePath;
    if (const QString objFile = match.captured("obj"); !objFile.isEmpty()) {
        objFilePath = absoluteFilePath(FilePath::fromUserInput(objFile));
        addLinkSpecForAbsoluteFilePath(linkSpecs, objFilePath, -1, -1, match, "obj");
    }

    if (!hasKeyword && !match.hasCaptured("cmd") && objFilePath.isEmpty())
        return {};

    int lineno = -1;
    if (const QString lineStr = match.captured("line"); !lineStr.isEmpty())
        lineno = lineStr.toInt();
    FilePath srcFilePath;
    if (const QString srcFile = match.captured("src"); !srcFile.isEmpty()) {
        srcFilePath = absoluteFilePath(FilePath::fromUserInput(srcFile));
        addLinkSpecForAbsoluteFilePath(linkSpecs, srcFilePath, lineno, -1, match, "src");
    }

    Task::TaskType type = hasKeyword ? Task::Error : Task::Unknown;
    const QString warningPrefix = "warning: ";
    for (const QString &prefix : QStringList{warningPrefix, "error: ", "fatal: "}) {
        if (description.startsWith(prefix)) {
            description = description.mid(prefix.size());
            type = prefix == warningPrefix ? Task::Warning : Task::Error;
            break;
        }
    }
    const FilePath filePath = !srcFilePath.isEmpty() ? srcFilePath : objFilePath;
    createOrAmendTask(type, description, originalLine, false, filePath, lineno, 0, linkSpecs);
    return Result(getStatus(trimmedLine), linkSpecs);
}

OutputLineParser::Status LdParser::getStatus(const QString &line)
{
    return line.endsWith(':') ? Status::InProgress : Status::Done;
};

#ifdef WITH_TESTS
class LdOutputParserTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<OutputParserTester::Channel>("inputChannel");
        QTest::addColumn<QStringList>("childStdOutLines");
        QTest::addColumn<QStringList>("childStdErrLines");
        QTest::addColumn<Tasks >("tasks");

        const auto formatRange = [](int start, int length, const QString &anchorHref = {})
        {
            QTextCharFormat format;
            format.setAnchorHref(anchorHref);
            return QTextLayout::FormatRange{start, length, format};
        };

        const auto compileTask = [](Task::TaskType type,
                                    const QString &description,
                                    const Utils::FilePath &file,
                                    int line,
                                    int column,
                                    const QList<QTextLayout::FormatRange> formats) {
            CompileTask task(type, description, file, line, column);
            task.formats = formats;
            return task;
        };

        QList<QTextLayout::FormatRange> formatRanges;
        if (HostOsInfo::isWindowsHost()) {
            formatRanges << formatRange(51, 28)
            << formatRange(79, 31, "olpfile://C:/temp/test/untitled8/main.cpp::8::-1")
            << formatRange(110, 54);
        } else {
            formatRanges << formatRange(51, 113);
        }
        QTest::newRow("Undefined reference (debug)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:8: undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << compileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'\n"
                               "main.o: In function `main':\n"
                               "C:\\temp\\test\\untitled8/main.cpp:8: undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8/main.cpp"),
                               8, 0,
                               formatRanges)
                << CompileTask(Task::Error, "collect2: ld returned 1 exit status"));

        formatRanges.clear();
        if (HostOsInfo::isWindowsHost()) {
            formatRanges << formatRange(51, 28)
            << formatRange(79, 31, "olpfile://C:/temp/test/untitled8/main.cpp::-1::-1")
            << formatRange(110, 65);
        } else {
            formatRanges << formatRange(51, 124);
        }
        QTest::newRow("Undefined reference (release)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << compileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'\n"
                               "main.o: In function `main':\n"
                               "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8/main.cpp"),
                               -1, 0,
                               formatRanges)
                << CompileTask(Task::Error, "collect2: ld returned 1 exit status"));

        QTest::newRow("linker: dll format not recognized")
            << QString::fromLatin1("c:\\Qt\\4.6\\lib/QtGuid4.dll: file not recognized: File format not recognized")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Error,
                               "file not recognized: File format not recognized",
                               FilePath::fromUserInput("c:\\Qt\\4.6\\lib/QtGuid4.dll")));

        QTest::newRow("ld warning (QTCREATORBUG-905)")
            << QString::fromLatin1("ld: warning: Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o"));

        QTest::newRow("ld fatal")
            << QString::fromLatin1("ld: fatal: Symbol referencing errors. No output written to testproject")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Error,
                               "Symbol referencing errors. No output written to testproject"));

        QTest::newRow("Linker fail (release build)")
            << QString::fromLatin1("release/main.o:main.cpp:(.text+0x42): undefined reference to `MainWindow::doSomething()'")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("main.cpp")));

        QTest::newRow("linker error") // QTCREATORBUG-3107
            << QString::fromLatin1("cns5k_ins_parser_tests.cpp:(.text._ZN20CNS5kINSParserEngine21DropBytesUntilStartedEP14CircularBufferIhE[CNS5kINSParserEngine::DropBytesUntilStarted(CircularBuffer<unsigned char>*)]+0x6d): undefined reference to `CNS5kINSPacket::SOH_BYTE'")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `CNS5kINSPacket::SOH_BYTE'",
                               FilePath::fromUserInput("cns5k_ins_parser_tests.cpp")));

        QTest::newRow("libimf warning")
            << QString::fromLatin1("libimf.so: warning: warning: feupdateenv is not implemented and will always fail")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "warning: feupdateenv is not implemented and will always fail",
                               FilePath::fromUserInput("libimf.so")));

        formatRanges.clear();
        if (HostOsInfo::isWindowsHost()) {
            formatRanges << formatRange(46, 44)
            << formatRange(90, 39, "olpfile://M:/Development/x64/QtPlot/qplotaxis.cpp::26::-1")
            << formatRange(129, 50);
        } else {
            formatRanges << formatRange(46, 133);
        }
        QTest::newRow("QTCREATORBUG-597")
            << QString::fromLatin1("debug/qplotaxis.o: In function `QPlotAxis':\n"
                                   "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'\n"
                                   "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << compileTask(Task::Error,
                               "undefined reference to `vtable for QPlotAxis'\n"
                               "debug/qplotaxis.o: In function `QPlotAxis':\n"
                               "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'",
                               FilePath::fromUserInput("M:\\Development\\x64\\QtPlot/qplotaxis.cpp"),
                               26, 0,
                               formatRanges)
                << CompileTask(Task::Error,
                               "undefined reference to `vtable for QPlotAxis'",
                               FilePath::fromUserInput("M:\\Development\\x64\\QtPlot/qplotaxis.cpp"),
                               26)
                << CompileTask(Task::Error,
                               "collect2: ld returned 1 exit status"));

        QTest::newRow("ld: Multiple definition error")
            << QString::fromLatin1("foo.o: In function `foo()':\n"
                                   "/home/user/test/foo.cpp:2: multiple definition of `foo()'\n"
                                   "bar.o:/home/user/test/bar.cpp:4: first defined here\n"
                                   "collect2: error: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << compileTask(Task::Error,
                               "multiple definition of `foo()'\n"
                               "foo.o: In function `foo()':\n"
                               "/home/user/test/foo.cpp:2: multiple definition of `foo()'",
                               FilePath::fromUserInput("/home/user/test/foo.cpp"),
                               2, 0,
                               QList<QTextLayout::FormatRange>()
                                   << formatRange(31, 28)
                                   << formatRange(59, 23, "olpfile:///home/user/test/foo.cpp::2::-1")
                                   << formatRange(82, 34))
                << CompileTask(Task::Error,
                               "first defined here",
                               FilePath::fromUserInput("/home/user/test/bar.cpp"),
                               4)
                << CompileTask(Task::Error,
                               "collect2: error: ld returned 1 exit status"));

        QTest::newRow("ld: .data section")
            << QString::fromLatin1("foo.o:(.data+0x0): multiple definition of `foo'\n"
                                   "bar.o:(.data+0x0): first defined here\n"
                                   "collect2: error: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Error,
                               "multiple definition of `foo'",
                               FilePath::fromUserInput("foo.o"), -1)
                << CompileTask(Task::Error,
                               "first defined here",
                               FilePath::fromUserInput("bar.o"), -1)
                << CompileTask(Task::Error,
                               "collect2: error: ld returned 1 exit status"));

        QTest::newRow("Undefined symbol (Apple ld)")
            << "Undefined symbols for architecture x86_64:\n"
               "  \"SvgLayoutTest()\", referenced from:\n"
               "      _main in main.cpp.o"
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << Tasks({CompileTask(Task::Error,
                                  "Undefined symbols for architecture x86_64:\n"
                                  "Undefined symbols for architecture x86_64:\n"
                                  "  \"SvgLayoutTest()\", referenced from:\n"
                                  "      _main in main.cpp.o",
                                  "main.cpp.o")});

        QTest::newRow("ld: undefined member function reference")
            << "obj/gtest-clang-printing.o:gtest-clang-printing.cpp:llvm::VerifyDisableABIBreakingChecks: error: undefined reference to 'llvm::DisableABIBreakingChecks'"
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to 'llvm::DisableABIBreakingChecks'",
                               "gtest-clang-printing.cpp"));

        QTest::newRow("Mac: ranlib warning")
            << QString::fromLatin1("ranlib: file: lib/libtest.a(Test0.cpp.o) has no symbols")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "file: lib/libtest.a(Test0.cpp.o) has no symbols"));

        QTest::newRow("Mac: ranlib warning2")
            << QString::fromLatin1("/path/to/XCode/and/ranlib: file: lib/libtest.a(Test0.cpp.o) has no symbols")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "file: lib/libtest.a(Test0.cpp.o) has no symbols"));

        QTest::newRow("ld: missing library")
            << QString::fromLatin1("/usr/bin/ld: cannot find -ldoesnotexist")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << CompileTask(Task::Error,
                               "cannot find -ldoesnotexist"));

        QTest::newRow("QTCREATORBUG-32502")
            << QString::fromLatin1(
                   R"(gcc -c -pipe -fPIC -g -Wall -Wextra -DQT_QML_DEBUG -I../../../ExternC -I. -I/opt/Qt/6.7.3/gcc_64/mkspecs/linux-g++ -o main.o ../../main.c
g++  -o ExternC  main.o
/usr/bin/ld: main.o: in function `main':
/home/andre/tmp/ExternC/build/Desktop_Qt_6_7_3-Debug/../../main.c:7: undefined reference to `foo'
/usr/bin/ld: /home/andre/tmp/ExternC/build/Desktop_Qt_6_7_3-Debug/../../main.c:9: undefined reference to `bar'
collect2: error: ld returned 1 exit status
make: *** [Makefile:245: ExternC] Error 1
17:48:20: The process "/usr/bin/make" exited with code 2.
17:48:20: Error while building/deploying project ExternC (kit: Desktop Qt 6.7.3)
17:48:20: When executing step "Make")")
            << OutputParserTester::STDERR << QStringList()
            << QStringList{
                "gcc -c -pipe -fPIC -g -Wall -Wextra -DQT_QML_DEBUG -I../../../ExternC -I. -I/opt/Qt/6.7.3/gcc_64/mkspecs/linux-g++ -o main.o ../../main.c",
                "g++  -o ExternC  main.o", "make: *** [Makefile:245: ExternC] Error 1",
                R"(17:48:20: The process "/usr/bin/make" exited with code 2.)",
                "17:48:20: Error while building/deploying project ExternC (kit: Desktop Qt 6.7.3)",
                R"(17:48:20: When executing step "Make")"}
            << Tasks{
                   CompileTask(
                       Task::Error,
                       "undefined reference to `foo'\n"
                       "/usr/bin/ld: main.o: in function `main':\n"
                       "/home/andre/tmp/ExternC/build/Desktop_Qt_6_7_3-Debug/../../main.c:7: undefined reference to `foo'",
                       "/home/andre/tmp/ExternC/main.c",
                       7),
                   CompileTask(
                       Task::Error,
                       "undefined reference to `bar'",
                       "/home/andre/tmp/ExternC/main.c",
                       9),
                   CompileTask(Task::Error, "collect2: error: ld returned 1 exit status")};

        const auto task = [](Task::TaskType type, const QString &msg,
                             const QString &file = {}, int line = -1) {
            return CompileTask(type, msg, FilePath::fromString(file), line);
        };
        const auto errorTask = [&task](const QString &msg, const QString &file = {}, int line = -1) {
            return task(Task::Error, msg, file, line);
        };
        const auto unknownTask = [&task](const QString &msg, const QString &file = {}, int line = -1) {
            return task(Task::Unknown, msg, file, line);
        };
        QTest::newRow("lld: undefined reference with debug info")
            << "ld.lld: error: undefined symbol: func()\n"
               ">>> referenced by test.cpp:5\n"
               ">>>               /tmp/ccg8pzRr.o:(main)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QStringList() << QStringList()
            << Tasks({
                      errorTask("ld.lld: error: undefined symbol: func()"),
                      unknownTask("referenced by test.cpp:5", "test.cpp", 5),
                      unknownTask("/tmp/ccg8pzRr.o:(main)",  "/tmp/ccg8pzRr.o"),
                      errorTask("collect2: error: ld returned 1 exit status")});

        QTest::newRow("lld: undefined reference with debug info (more verbose format)")
            << "ld.lld: error: undefined symbol: someFunc()\n"
               ">>> referenced by main.cpp:10 (/tmp/untitled4/main.cpp:10)\n"
               ">>>               /tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o:(main)\n"
               "clang-8: error: linker command failed with exit code 1 (use -v to see invocation)"
            << OutputParserTester::STDERR << QStringList()
            << QStringList("clang-8: error: linker command failed with exit code 1 (use -v to see invocation)")
            << Tasks{
                     errorTask("ld.lld: error: undefined symbol: someFunc()"),
                     unknownTask("referenced by main.cpp:10 (/tmp/untitled4/main.cpp:10)",
                                 "/tmp/untitled4/main.cpp", 10),
                     unknownTask("/tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o:(main)",
                                 "/tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o")};

        QTest::newRow("lld: undefined reference without debug info")
            << "ld.lld: error: undefined symbol: func()\n"
               ">>> referenced by test.cpp\n"
               ">>>               /tmp/ccvjyJph.o:(main)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QStringList() << QStringList()
            << Tasks{
                     errorTask("ld.lld: error: undefined symbol: func()"),
                     unknownTask("referenced by test.cpp", "test.cpp"),
                     unknownTask("/tmp/ccvjyJph.o:(main)",  "/tmp/ccvjyJph.o"),
                     errorTask("collect2: error: ld returned 1 exit status")};

        if (HostOsInfo::isWindowsHost()) {
            QTest::newRow("lld: undefined reference with mingw")
            << "lld-link: error: undefined symbol: __Z4funcv\n"
               ">>> referenced by C:\\Users\\orgads\\AppData\\Local\\Temp\\cccApKoz.o:(.text)\n"
               "collect2.exe: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QStringList() << QStringList()
            << Tasks{
                     errorTask("lld-link: error: undefined symbol: __Z4funcv"),
                     unknownTask("referenced by C:\\Users\\orgads\\AppData\\Local\\Temp\\cccApKoz.o:(.text)",
                                 "C:/Users/orgads/AppData/Local/Temp/cccApKoz.o"),
                     errorTask("collect2.exe: error: ld returned 1 exit status")};
        }

        QTest::newRow("lld: multiple definitions with debug info")
            << "ld.lld: error: duplicate symbol: func()\n"
               ">>> defined at test1.cpp:1\n"
               ">>>            test1.o:(func())\n"
               ">>> defined at test1.cpp:1\n"
               ">>>            test1.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QStringList() << QStringList()
            << Tasks{
                     errorTask("ld.lld: error: duplicate symbol: func()"),
                     unknownTask("defined at test1.cpp:1", "test1.cpp", 1),
                     unknownTask("test1.o:(func())",  "test1.o"),
                     unknownTask("defined at test1.cpp:1", "test1.cpp", 1),
                     unknownTask("test1.o:(.text+0x0)",  "test1.o"),
                     errorTask("collect2: error: ld returned 1 exit status")};

        QTest::newRow("lld: multiple definitions with debug info (more verbose format)")
            << "ld.lld: error: duplicate symbol: theFunc()\n"
               ">>> defined at file.cpp:1 (/tmp/untitled3/file.cpp:1)\n"
               ">>>            /tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/file.cpp.o:(theFunc())\n"
               ">>> defined at main.cpp:5 (/tmp/untitled3/main.cpp:5)\n"
               ">>>            /tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/main.cpp.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QStringList() << QStringList()
            << Tasks{
                     errorTask("ld.lld: error: duplicate symbol: theFunc()"),
                     unknownTask("defined at file.cpp:1 (/tmp/untitled3/file.cpp:1)",
                                 "/tmp/untitled3/file.cpp", 1),
                     unknownTask("/tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/file.cpp.o:(theFunc())",
                                 "/tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/file.cpp.o"),
                     unknownTask("defined at main.cpp:5 (/tmp/untitled3/main.cpp:5)",
                                 "/tmp/untitled3/main.cpp", 5),
                     unknownTask("/tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/main.cpp.o:(.text+0x0)",
                                 "/tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/main.cpp.o"),
                     errorTask("collect2: error: ld returned 1 exit status")};

        QTest::newRow("lld: multiple definitions without debug info")
            << "ld.lld: error: duplicate symbol: func()\n"
               ">>> defined at test1.cpp\n"
               ">>>            test1.o:(func())\n"
               ">>> defined at test1.cpp\n"
               ">>>            test1.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QStringList() << QStringList()
            << Tasks{
                     errorTask("ld.lld: error: duplicate symbol: func()"),
                     unknownTask("defined at test1.cpp", "test1.cpp"),
                     unknownTask("test1.o:(func())",  "test1.o"),
                     unknownTask("defined at test1.cpp", "test1.cpp"),
                     unknownTask("test1.o:(.text+0x0)",  "test1.o"),
                     errorTask("collect2: error: ld returned 1 exit status")};

        if (HostOsInfo::isWindowsHost()) {
            QTest::newRow("lld: multiple definitions with mingw")
            << "lld-link: error: duplicate symbol: __Z4funcv in test1.o and in test2.o\n"
               "collect2.exe: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QStringList() << QStringList()
            << Tasks{
                     errorTask("lld-link: error: duplicate symbol: __Z4funcv in test1.o and in test2.o"),
                     errorTask("collect2.exe: error: ld returned 1 exit status", {})};
        }

        QTest::newRow("mixed location")
            << "/usr/lib/gcc/x86_64-pc-linux-gnu/14/../../../../x86_64-pc-linux-gnu/bin/ld: Build.dir/Target.dir/path/to/source_file.cpp.o: in function `foo()':\n"
               "/path/to/source_file.cpp:42:(.text+0xb58): undefined reference to `bar()'\n"
               "/usr/lib/gcc/x86_64-pc-linux-gnu/14/../../../../x86_64-pc-linux-gnu/bin/ld: /path/to/source_file.cpp:138:(.text+0xbc2): undefined reference to `bar()'"
            << OutputParserTester::STDERR << QStringList() << QStringList()
            << Tasks{
                   errorTask("undefined reference to `bar()'\n"
                             "/usr/lib/gcc/x86_64-pc-linux-gnu/14/../../../../x86_64-pc-linux-gnu/bin/ld: Build.dir/Target.dir/path/to/source_file.cpp.o: in function `foo()':\n"
                             "/path/to/source_file.cpp:42:(.text+0xb58): undefined reference to `bar()'",
                             "/path/to/source_file.cpp", 42),
                   errorTask("undefined reference to `bar()'", "/path/to/source_file.cpp", 138)};
    }

    void test()
    {
        OutputParserTester testbench;
        testbench.setLineParsers({new LdParser, new LldParser});
        QFETCH(QString, input);
        QFETCH(OutputParserTester::Channel, inputChannel);
        QFETCH(Tasks, tasks);
        QFETCH(QStringList, childStdOutLines);
        QFETCH(QStringList, childStdErrLines);

        testbench.testParsing(input, inputChannel, tasks, childStdOutLines, childStdErrLines);
    }
};

QObject *createLdOutputParserTest()
{
    return new LdOutputParserTest;
}
#endif // WITH_TESTS

} // namespace ProjectExplorer::Internal

#ifdef WITH_TESTS
#include <ldparser.moc>
#endif // WITH_TESTS
