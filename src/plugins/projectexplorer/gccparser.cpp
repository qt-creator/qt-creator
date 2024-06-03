// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gccparser.h"
#include "ldparser.h"
#include "lldparser.h"
#include "task.h"

#include <utils/qtcassert.h>

#include <QLoggingCategory>

#include <optional>

using namespace Utils;

namespace ProjectExplorer {

static Q_LOGGING_CATEGORY(gccParserLog, "qtc.gccparser", QtWarningMsg)

static const QString &filePattern()
{
    static const QString pattern = [] {
        const QString pseudoFile = "<command[ -]line>";
        const QString driveSpec = "[A-Za-z]:";
        const QString realFile = QString::fromLatin1("(?:%1)?[^:]+").arg(driveSpec);
        return QString::fromLatin1("(?<file>%1|%2):").arg(pseudoFile, realFile);
    }();
    return pattern;
}

static const char COMMAND_PATTERN[] = "^(.*?[\\\\/])?([a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(gcc|g\\+\\+)(-[0-9.]+)?(\\.exe)?: ";

namespace {
class MainRegEx
{
public:
    struct Data {
        QString rawFilePath;
        QString description;
        Task::TaskType type = Task::Unknown;
        int line = -1;
        int column = -1;
        int fileOffset = -1;
    };

    static std::optional<Data> parse(const QString &line)
    {
        qCDebug(gccParserLog) << "checking regex" << theRegEx().pattern();
        const QRegularExpressionMatch match = theRegEx().match(line);
        if (!match.hasMatch())
            return {};

        // Quick plausability test: If there's no slashes or dots, it's probably not a file.
        const QString possibleFile = match.captured("file");
        if (!possibleFile.contains('/') && !possibleFile.contains("'\\")
            && !possibleFile.contains('.')) {
            return {};
        }

        // Defer to the LdParser for some file types
        if (possibleFile.endsWith(".o") || possibleFile.endsWith(".a")
            || possibleFile.endsWith("dll") || possibleFile.contains(".so")
            || possibleFile.endsWith("ld") || possibleFile.endsWith("ranlib")) {
            return {};
        }

        Data data;
        data.rawFilePath = possibleFile;
        data.fileOffset = match.capturedStart("file");
        data.line = match.captured("line").toInt();
        data.column = match.captured("column").toInt();
        data.description = match.captured("description");
        if (match.captured("type") == QLatin1String("warning")) {
            data.type = Task::Warning;
        } else if (match.captured("type") == QLatin1String("error") ||
                   data.description.startsWith(QLatin1String("undefined reference to")) ||
                   data.description.startsWith(QLatin1String("multiple definition of"))) {
            data.type = Task::Error;
        }

        // Prepend "#warning" or "#error" if that triggered the match on (warning|error)
        // We want those to show how the warning was triggered
        if (match.captured("fullTypeString").startsWith(QLatin1Char('#')))
            data.description.prepend(match.captured("fullTypeString"));

        return data;
    }

private:
    static const QRegularExpression &theRegEx()
    {
        static const QRegularExpression re = [] {
            const QRegularExpression re(constructPattern());
            QTC_CHECK(re.isValid());
            return re;
        }();
        return re;
    }

    static QString constructPattern()
    {
        const QString type = "(?<type>warning|error|note)";
        const QString typePrefix = "(?:fatal |#)";
        const QString fullTypeString
            = QString::fromLatin1("(?<fullTypeString>%1?%2:?\\s)").arg(typePrefix, type);
        const QString optionalLineAndColumn = "(?:(?:(?<line>\\d+)(?::(?<column>\\d+))?):)?";
        const QString binaryLocation = "\\(.*\\)"; // E.g. "(.text+0x40)"
        const QString fullLocation = QString::fromLatin1("%1(?:%2|%3)")
                                         .arg(filePattern(), optionalLineAndColumn, binaryLocation);
        const QString description = "(?<description>[^\\s].+)";

        return QString::fromLatin1("^%1\\s+%2?%3$").arg(fullLocation, fullTypeString, description);
    }
};
} // namespace

GccParser::GccParser()
{
    setObjectName(QLatin1String("GCCParser"));

    m_regExpIncluded.setPattern(QString::fromLatin1("\\bfrom\\s") + filePattern()
                                + QLatin1String("(\\d+)(:\\d+)?[,:]?$"));
    QTC_CHECK(m_regExpIncluded.isValid());

    m_regExpCc1plus.setPattern(QLatin1Char('^') + "cc1plus.*(error|warning): ((?:"
                               + filePattern() + " No such file or directory)?.*)");
    QTC_CHECK(m_regExpCc1plus.isValid());

    // optional path with trailing slash
    // optional arm-linux-none-thingy
    // name of executable
    // optional trailing version number
    // optional .exe postfix
    m_regExpGccNames.setPattern(QLatin1String(COMMAND_PATTERN));
    QTC_CHECK(m_regExpGccNames.isValid());
}

Utils::Id GccParser::id()
{
    return Utils::Id("ProjectExplorer.OutputParser.Gcc");
}

QList<OutputLineParser *> GccParser::gccParserSuite()
{
    return {new GccParser, new Internal::LldParser, new LdParser};
}

void GccParser::gccCreateOrAmendTask(
    Task::TaskType type,
    const QString &description,
    const QString &originalLine,
    bool forceAmend,
    const Utils::FilePath &file,
    int line,
    int column,
    const LinkSpecs &linkSpecs)
{
    createOrAmendTask(type, description, originalLine, forceAmend, file, line, column, linkSpecs);

    // If a "required from here" line is present, it is almost always the cause of the problem,
    // so that's where we should go when the issue is double-clicked.
    if ((originalLine.endsWith("required from here") || originalLine.endsWith("requested here")
         || originalLine.endsWith("note: here"))
        && !file.isEmpty() && line > 0) {
        fixTargetLink();
        currentTask().setFile(file);
        currentTask().line = line;
        currentTask().column = column;
    }
}

OutputLineParser::Result GccParser::handleLine(const QString &line, OutputFormat type)
{
    qCDebug(gccParserLog) << "incoming line" << line;

    if (type == StdOutFormat) {
        qCDebug(gccParserLog) << "not parsing stdout";
        flush();
        return Status::NotHandled;
    }

    const QString lne = rightTrimmed(line);

    // Blacklist some lines to not handle them:
    if (lne.startsWith(QLatin1String("TeamBuilder "))
        || lne.startsWith(QLatin1String("distcc["))
        || lne.contains("undefined reference")
        || lne.contains("undefined symbol")
        || lne.contains("duplicate symbol")
        || lne.contains("multiple definition")
        || lne.contains("ar: creating")) {
        return Status::NotHandled;
    }

    // Handle misc issues:
    if (lne.startsWith(QLatin1String("ERROR:")) || lne == QLatin1String("* cpp failed")) {
        gccCreateOrAmendTask(Task::Error, lne, lne);
        return Status::InProgress;
    }

    qCDebug(gccParserLog) << "checking regex" << m_regExpGccNames.pattern();
    QRegularExpressionMatch match = m_regExpGccNames.match(lne);
    if (match.hasMatch()) {
        QString description = lne.mid(match.capturedLength());
        Task::TaskType type = Task::Error;
        if (description.startsWith(QLatin1String("warning: "))) {
            type = Task::Warning;
            description = description.mid(8);
        } else if (description.startsWith(QLatin1String("fatal: ")))  {
            description = description.mid(6);
        }
        gccCreateOrAmendTask(type, description, lne);
        return Status::InProgress;
    }

    qCDebug(gccParserLog) << "checking regex" << m_regExpIncluded.pattern();
    match = m_regExpIncluded.match(lne);
    if (match.hasMatch()) {
        const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured("file")));
        const int lineNo = match.captured(2).toInt();
        const int column = match.captured(3).toInt();
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineNo, column, match, "file");
        gccCreateOrAmendTask(
            Task::Unknown, lne.trimmed(), lne, false, filePath, lineNo, column, linkSpecs);
        return {Status::InProgress, linkSpecs};
    }

    qCDebug(gccParserLog) << "checking regex" << m_regExpCc1plus.pattern();
    match = m_regExpCc1plus.match(lne);
    if (match.hasMatch()) {
        const Task::TaskType type = match.captured(1) == "error" ? Task::Error : Task::Warning;
        const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured(3)));
        LinkSpecs linkSpecs;
        if (!filePath.isEmpty())
            addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, -1, -1, match, 3);
        gccCreateOrAmendTask(type, match.captured(2), lne, false, filePath, -1, 0, linkSpecs);
        return {Status::Done, linkSpecs};
    }

    if (const auto data = MainRegEx::parse(lne)) {
        const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(data->rawFilePath));
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(
            linkSpecs,
            filePath,
            data->line,
            data->column,
            data->fileOffset,
            data->rawFilePath.size());
        gccCreateOrAmendTask(
            data->type, data->description, lne, false, filePath, data->line, data->column, linkSpecs);
        return {Status::InProgress, linkSpecs};
    }

    if ((lne.startsWith(' ') && !currentTask().isNull()) || isContinuation(lne)) {
        gccCreateOrAmendTask(Task::Unknown, lne, lne, true);
        return Status::InProgress;
    }

    qCDebug(gccParserLog) << "no match";
    flush();
    return Status::NotHandled;
}

bool GccParser::isContinuation(const QString &newLine) const
{
    return !currentTask().isNull()
           && (currentTask().details.last().endsWith(':')
               || currentTask().details.last().endsWith(',')
               || currentTask().details.last().contains(" required from ")
               || newLine.contains("within this context")
               || newLine.contains("note:"));
}

} // ProjectExplorer

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer_test.h"
#   include "outputparser_test.h"

namespace ProjectExplorer::Internal {

void ProjectExplorerTest::testGccOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<Tasks >("tasks");
    QTest::addColumn<QString>("outputLines");

    auto compileTask = [](Task::TaskType type,
                          const QString &description,
                          const Utils::FilePath &file,
                          int line,
                          int column,
                          const QVector<QTextLayout::FormatRange> formats)
    {
        CompileTask task(type, description, file, line, column);
        task.formats = formats;
        return task;
    };

    auto formatRange = [](int start, int length, const QString &anchorHref = QString())
    {
        QTextCharFormat format;
        format.setAnchorHref(anchorHref);

        return QTextLayout::FormatRange{start, length, format};
    };

    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << Tasks()
            << QString();

    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << Tasks()
            << QString();

    QTest::newRow("ar output")
            << QString::fromLatin1("../../../../x86/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-ar: creating lib/libSkyView.a") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("../../../../x86/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-ar: creating lib/libSkyView.a\n")
            << Tasks()
            << QString();

    QTest::newRow("GCCE error")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "/temp/test/untitled8/main.cpp:9: error: `sfasdf' undeclared (first use this function)\n"
                                   "/temp/test/untitled8/main.cpp:9: error: (Each undeclared identifier is reported only once for each function it appears in.)")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << compileTask(Task::Error,
                               "`sfasdf' undeclared (first use this function)\n"
                               "/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                               "/temp/test/untitled8/main.cpp:9: error: `sfasdf' undeclared (first use this function)",
                               FilePath::fromUserInput("/temp/test/untitled8/main.cpp"),
                               9, 0,
                               QVector<QTextLayout::FormatRange>()
                                   << formatRange(46, 0)
                                   << formatRange(46, 29, "olpfile:///temp/test/untitled8/main.cpp::0::0")
                                   << formatRange(75, 39)
                                   << formatRange(114, 29, "olpfile:///temp/test/untitled8/main.cpp::9::0")
                                   << formatRange(143, 56))
                << CompileTask(Task::Error,
                               "(Each undeclared identifier is reported only once for each function it appears in.)",
                               FilePath::fromUserInput("/temp/test/untitled8/main.cpp"),
                               9, 0))
            << QString();

    QTest::newRow("GCCE warning")
            << QString::fromLatin1("/src/corelib/global/qglobal.h:1635: warning: inline function `QDebug qDebug()' used but never defined")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "inline function `QDebug qDebug()' used but never defined",
                               FilePath::fromUserInput("/src/corelib/global/qglobal.h"),
                               1635, 0))
            << QString();

    QTest::newRow("warning")
            << QString::fromLatin1("main.cpp:7:2: warning: Some warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "Some warning",
                               FilePath::fromUserInput("main.cpp"),
                               7, 2))
            << QString();

    QTest::newRow("GCCE #error")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:7: #error Symbian error")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "#error Symbian error",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8\\main.cpp"),
                               7, 0))
            << QString();

    // Symbian reports #warning(s) twice (using different syntax).
    QTest::newRow("GCCE #warning1")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:8: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "#warning Symbian warning",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8\\main.cpp"),
                               8, 0))
            << QString();

    QTest::newRow("GCCE #warning2")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp:8:2: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "#warning Symbian warning",
                               FilePath::fromUserInput("/temp/test/untitled8/main.cpp"),
                               8, 2))
            << QString();

    QVector<QTextLayout::FormatRange> formatRanges;
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
            << QString() << QString()
            << (Tasks()
                << compileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'\n"
                               "main.o: In function `main':\n"
                               "C:\\temp\\test\\untitled8/main.cpp:8: undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8/main.cpp"),
                               8, 0,
                               formatRanges)
                << CompileTask(Task::Error, "collect2: ld returned 1 exit status"))
            << QString();

    formatRanges.clear();
    if (HostOsInfo::isWindowsHost()) {
        formatRanges << formatRange(51, 28)
                     << formatRange(79, 31, "olpfile://C:/temp/test/untitled8/main.cpp::0::-1")
                     << formatRange(110, 65);
    } else {
        formatRanges << formatRange(51, 124);
    }
    QTest::newRow("Undefined reference (release)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << compileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'\n"
                               "main.o: In function `main':\n"
                               "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8/main.cpp"),
                               -1, 0,
                               formatRanges)
                << CompileTask(Task::Error, "collect2: ld returned 1 exit status"))
            << QString();

    QTest::newRow("linker: dll format not recognized")
            << QString::fromLatin1("c:\\Qt\\4.6\\lib/QtGuid4.dll: file not recognized: File format not recognized")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "file not recognized: File format not recognized",
                               FilePath::fromUserInput("c:\\Qt\\4.6\\lib/QtGuid4.dll")))
            << QString();

    QTest::newRow("Invalid rpath")
            << QString::fromLatin1("g++: /usr/local/lib: No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "/usr/local/lib: No such file or directory"))
            << QString();

    QTest::newRow("unused variable")
            << QString::fromLatin1("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp: In member function 'void Debugger::Internal::GdbEngine::handleBreakInsert2(const Debugger::Internal::GdbResponse&)':\n"
                                   "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2114: warning: unused variable 'index'\n"
                                   "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2115: warning: unused variable 'handler'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << compileTask(Task::Warning,
                               "unused variable 'index'\n"
                               "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp: In member function 'void Debugger::Internal::GdbEngine::handleBreakInsert2(const Debugger::Internal::GdbResponse&)':\n"
                               "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2114: warning: unused variable 'index'",
                               FilePath::fromUserInput("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp"),
                               2114, 0,
                               QVector<QTextLayout::FormatRange>()
                                   << formatRange(24, 272))
                << CompileTask(Task::Warning,
                               "unused variable 'handler'",
                               FilePath::fromUserInput("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp"),
                               2115, 0))
            << QString();

    QTest::newRow("gnumakeparser.cpp errors")
            << QString::fromLatin1("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp: In member function 'void ProjectExplorer::ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()':\n"
                                   "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected primary-expression before ':' token\n"
                                   "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected ';' before ':' token")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << compileTask(Task::Error,
                               "expected primary-expression before ':' token\n"
                               "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp: In member function 'void ProjectExplorer::ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()':\n"
                               "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected primary-expression before ':' token",
                               FilePath::fromUserInput("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp"),
                               264, 0,
                               QVector<QTextLayout::FormatRange>()
                                   << formatRange(45, 0)
                                   << formatRange(45, 68, "olpfile:///home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp::0::0")
                                   << formatRange(113, 106)
                                   << formatRange(219, 68, "olpfile:///home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp::264::0")
                                   << formatRange(287, 57))
                << CompileTask(Task::Error,
                               "expected ';' before ':' token",
                               FilePath::fromUserInput("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp"),
                               264, 0))
            << QString();

    QTest::newRow("distcc error(QTCREATORBUG-904)")
            << QString::fromLatin1("distcc[73168] (dcc_get_hostlist) Warning: no hostlist is set; can't distribute work\n"
                                   "distcc[73168] (dcc_build_somewhere) Warning: failed to distribute, running locally instead")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("distcc[73168] (dcc_get_hostlist) Warning: no hostlist is set; can't distribute work\n"
                                                "distcc[73168] (dcc_build_somewhere) Warning: failed to distribute, running locally instead\n")
            << Tasks()
            << QString();

    QTest::newRow("ld warning (QTCREATORBUG-905)")
            << QString::fromLatin1("ld: warning: Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Warning,
                         "Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o"))
            << QString();

    QTest::newRow("ld fatal")
            << QString::fromLatin1("ld: fatal: Symbol referencing errors. No output written to testproject")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Error,
                         "Symbol referencing errors. No output written to testproject"))
            << QString();

    QTest::newRow("Teambuilder issues")
            << QString::fromLatin1("TeamBuilder Client:: error: could not find Scheduler, running Job locally...")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("TeamBuilder Client:: error: could not find Scheduler, running Job locally...\n")
            << Tasks()
            << QString();

    QTest::newRow("note")
            << QString::fromLatin1("/home/dev/creator/share/qtcreator/debugger/dumper.cpp:1079: note: initialized from here")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "initialized from here",
                                FilePath::fromUserInput("/home/dev/creator/share/qtcreator/debugger/dumper.cpp"),
                                1079))
            << QString();

    QTest::newRow("static member function")
            << QString::fromLatin1("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c: In static member function 'static std::_Rb_tree_node_base* std::_Rb_global<_Dummy>::_Rebalance_for_erase(std::_Rb_tree_node_base*, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&)':\n"
                                   "/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c:194: warning: suggest explicit braces to avoid ambiguous 'else'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << compileTask(Task::Warning,
                                "suggest explicit braces to avoid ambiguous 'else'\n"
                                "/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c: In static member function 'static std::_Rb_tree_node_base* std::_Rb_global<_Dummy>::_Rebalance_for_erase(std::_Rb_tree_node_base*, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&)':\n"
                                "/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c:194: warning: suggest explicit braces to avoid ambiguous 'else'",
                                FilePath::fromUserInput("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c"),
                                194, 0,
                                QVector<QTextLayout::FormatRange>()
                                    << formatRange(50, 0)
                                    << formatRange(50, 67, "olpfile:///Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c::0::0")
                                    << formatRange(117, 216)
                                    << formatRange(333, 67, "olpfile:///Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c::194::0")
                                    << formatRange(400, 64)))
            << QString();

    QTest::newRow("rm false positive")
            << QString::fromLatin1("rm: cannot remove `release/moc_mainwindow.cpp': No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("rm: cannot remove `release/moc_mainwindow.cpp': No such file or directory\n")
            << Tasks()
            << QString();

    QTest::newRow("ld: missing library")
            << QString::fromLatin1("/usr/bin/ld: cannot find -ldoesnotexist")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "cannot find -ldoesnotexist"))
            << QString();

    QTest::newRow("In function")
            << QString::fromLatin1("../../scriptbug/main.cpp: In function void foo(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n"
                                   "../../scriptbug/main.cpp:8: warning: unused variable c")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << compileTask(Task::Unknown,
                                "In function void foo(i) [with i = double]:\n"
                                "../../scriptbug/main.cpp: In function void foo(i) [with i = double]:\n"
                                "../../scriptbug/main.cpp:22: instantiated from here",
                                FilePath::fromUserInput("../../scriptbug/main.cpp"),
                                -1, 0,
                                QVector<QTextLayout::FormatRange>()
                                    << formatRange(43, 120))
                 << CompileTask(Task::Warning,
                                "unused variable c",
                                FilePath::fromUserInput("../../scriptbug/main.cpp"),
                                8))
            << QString();

    QTest::newRow("instantiated from here")
            << QString::fromLatin1("main.cpp:10: instantiated from here  ")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "instantiated from here",
                                FilePath::fromUserInput("main.cpp"),
                                10))
            << QString();

    QTest::newRow("In constructor")
            << QString::fromLatin1("/dev/creator/src/plugins/find/basetextfind.h: In constructor 'Find::BaseTextFind::BaseTextFind(QTextEdit*)':")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "In constructor 'Find::BaseTextFind::BaseTextFind(QTextEdit*)':",
                                FilePath::fromUserInput("/dev/creator/src/plugins/find/basetextfind.h")))
            << QString();

    QTest::newRow("At global scope")
            << QString::fromLatin1("../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n"
                                   "../../scriptbug/main.cpp:5: warning: unused parameter v")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << compileTask(Task::Unknown,
                                "At global scope:\n"
                                "../../scriptbug/main.cpp: At global scope:\n"
                                "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]",
                                FilePath::fromUserInput("../../scriptbug/main.cpp"),
                                -1, 0,
                                QVector<QTextLayout::FormatRange>()
                                    << formatRange(17, 195))
                << CompileTask(Task::Unknown,
                               "instantiated from here",
                               FilePath::fromUserInput("../../scriptbug/main.cpp"),
                               22)
                << CompileTask(Task::Warning,
                               "unused parameter v",
                               FilePath::fromUserInput("../../scriptbug/main.cpp"),
                               5))
            << QString();

    QTest::newRow("gcc 4.5 fatal error")
            << QString::fromLatin1("/home/code/test.cpp:54:38: fatal error: test.moc: No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Error,
                                "test.moc: No such file or directory",
                                FilePath::fromUserInput("/home/code/test.cpp"),
                                54, 38))
            << QString();

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
            << QString() << QString()
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
                               "collect2: ld returned 1 exit status"))
            << QString();

    QTest::newRow("instantiated from here should not be an error")
            << QString::fromLatin1("../stl/main.cpp: In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:\n"
                                   "../stl/main.cpp:38:   instantiated from here\n"
                                   "../stl/main.cpp:31: warning: returning reference to temporary\n"
                                   "../stl/main.cpp: At global scope:\n"
                                   "../stl/main.cpp:31: warning: unused parameter index")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << compileTask(Task::Unknown,
                               "In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:\n"
                               "../stl/main.cpp: In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:\n"
                               "../stl/main.cpp:38:   instantiated from here",
                               FilePath::fromUserInput("../stl/main.cpp"),
                               -1, 0,
                               QVector<QTextLayout::FormatRange>()
                                   << formatRange(163, 224))
                << CompileTask(Task::Warning,
                               "returning reference to temporary",
                               FilePath::fromUserInput("../stl/main.cpp"), 31)
                << compileTask(Task::Warning,
                               "unused parameter index\n"
                               "../stl/main.cpp: At global scope:\n"
                               "../stl/main.cpp:31: warning: unused parameter index",
                               FilePath::fromUserInput("../stl/main.cpp"),
                               31, 0,
                               QVector<QTextLayout::FormatRange>()
                                   << formatRange(23, 85)))
            << QString();

    formatRanges.clear();
    if (HostOsInfo::isWindowsHost()) {
        formatRanges << formatRange(33, 22)
                     << formatRange(55, 38, "olpfile://C:/Symbian_SDK/epoc32/include/e32cmn.h::6792::-1")
                     << formatRange(93, 29)
                     << formatRange(122, 38, "olpfile://C:/Symbian_SDK/epoc32/include/e32std.h::25::-1")
                     << formatRange(160, 5)
                     << formatRange(165, 40, "olpfile://C:/Symbian_SDK/epoc32/include/e32cmn.inl::0::-1")
                     << formatRange(205, 69)
                     << formatRange(274, 40, "olpfile://C:/Symbian_SDK/epoc32/include/e32cmn.inl::7094::-1")
                     << formatRange(314, 48);
    } else {
        formatRanges << formatRange(33, 329);
    }
    QTest::newRow("GCCE from lines")
            << QString::fromLatin1("In file included from C:/Symbian_SDK/epoc32/include/e32cmn.h:6792,\n"
                                   "                 from C:/Symbian_SDK/epoc32/include/e32std.h:25,\n"
                                   "C:/Symbian_SDK/epoc32/include/e32cmn.inl: In member function 'SSecureId::operator const TSecureId&() const':\n"
                                   "C:/Symbian_SDK/epoc32/include/e32cmn.inl:7094: warning: returning reference to temporary")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(Task::Warning,
                                 "returning reference to temporary\n"
                                 "In file included from C:/Symbian_SDK/epoc32/include/e32cmn.h:6792,\n"
                                 "                 from C:/Symbian_SDK/epoc32/include/e32std.h:25,\n"
                                 "C:/Symbian_SDK/epoc32/include/e32cmn.inl: In member function 'SSecureId::operator const TSecureId&() const':\n"
                                 "C:/Symbian_SDK/epoc32/include/e32cmn.inl:7094: warning: returning reference to temporary",
                                 FilePath::fromUserInput("C:/Symbian_SDK/epoc32/include/e32cmn.inl"),
                                 7094, 0,
                                 formatRanges)}
            << QString();
    QTest::newRow("In constructor 2")
            << QString::fromUtf8("perfattributes.cpp: In constructor ‘PerfEventAttributes::PerfEventAttributes()’:\n"
                                 "perfattributes.cpp:28:48: warning: ‘void* memset(void*, int, size_t)’ clearing an object of non-trivial type ‘class PerfEventAttributes’; use assignment or value-initialization instead [-Wclass-memaccess]\n"
                                 "   28 |     memset(this, 0, sizeof(PerfEventAttributes));\n"
                                 "      |                                                ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(Task::Warning,
                                 "‘void* memset(void*, int, size_t)’ clearing an object of non-trivial type ‘class PerfEventAttributes’; use assignment or value-initialization instead [-Wclass-memaccess]\n"
                                 "perfattributes.cpp: In constructor ‘PerfEventAttributes::PerfEventAttributes()’:\n"
                                 "perfattributes.cpp:28:48: warning: ‘void* memset(void*, int, size_t)’ clearing an object of non-trivial type ‘class PerfEventAttributes’; use assignment or value-initialization instead [-Wclass-memaccess]\n"
                                 "   28 |     memset(this, 0, sizeof(PerfEventAttributes));\n"
                                 "      |                                                ^",
                                 FilePath::fromUserInput("perfattributes.cpp"),
                                 28, 48,
                                 QVector<QTextLayout::FormatRange>()
                                     << formatRange(170, 400))}
            << QString();
    QTest::newRow("QTCREATORBUG-2206")
            << QString::fromLatin1("../../../src/XmlUg/targetdelete.c: At top level:")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "At top level:",
                                FilePath::fromUserInput("../../../src/XmlUg/targetdelete.c")))
            << QString();

    QTest::newRow("GCCE 4: commandline, includes")
            << QString::fromLatin1("In file included from /Symbian/SDK/EPOC32/INCLUDE/GCCE/GCCE.h:15,\n"
                                   "                 from <command line>:26:\n"
                                   "/Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh:1134:26: warning: no newline at end of file")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(
                      Task::Warning,
                      "no newline at end of file\n"
                      "In file included from /Symbian/SDK/EPOC32/INCLUDE/GCCE/GCCE.h:15,\n"
                      "                 from <command line>:26:\n"
                      "/Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh:1134:26: warning: no newline at end of file",
                      FilePath::fromUserInput("/Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh"),
                      1134, 26,
                      QVector<QTextLayout::FormatRange>()
                          << formatRange(26, 22)
                          << formatRange(48, 39, "olpfile:///Symbian/SDK/EPOC32/INCLUDE/GCCE/GCCE.h::15::0")
                          << formatRange(87, 46)
                          << formatRange(133, 50, "olpfile:///Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh::1134::26")
                          << formatRange(183, 44))}
            << QString();

    QTest::newRow("Linker fail (release build)")
            << QString::fromLatin1("release/main.o:main.cpp:(.text+0x42): undefined reference to `MainWindow::doSomething()'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("main.cpp")))
            << QString();

    QTest::newRow("enumeration warning")
            << QString::fromLatin1("../../../src/shared/proparser/profileevaluator.cpp: In member function 'ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(const ProString&, const ProStringList&)':\n"
                                   "../../../src/shared/proparser/profileevaluator.cpp:2817:9: warning: case value '0' not in enumerated type 'ProFileEvaluator::Private::TestFunc'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << compileTask(Task::Warning,
                                "case value '0' not in enumerated type 'ProFileEvaluator::Private::TestFunc'\n"
                                "../../../src/shared/proparser/profileevaluator.cpp: In member function 'ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(const ProString&, const ProStringList&)':\n"
                                "../../../src/shared/proparser/profileevaluator.cpp:2817:9: warning: case value '0' not in enumerated type 'ProFileEvaluator::Private::TestFunc'",
                                FilePath::fromUserInput("../../../src/shared/proparser/profileevaluator.cpp"),
                                2817, 9,
                                QVector<QTextLayout::FormatRange>()
                                    << formatRange(76, 351)))
            << QString();

    QTest::newRow("include with line:column info")
            << QString::fromLatin1("In file included from <command-line>:0:0:\n"
                                   "./mw.h:4:0: warning: \"STUPID_DEFINE\" redefined")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(
                   Task::Warning,
                   "\"STUPID_DEFINE\" redefined\n"
                   "In file included from <command-line>:0:0:\n"
                   "./mw.h:4:0: warning: \"STUPID_DEFINE\" redefined",
                   FilePath::fromUserInput("./mw.h"),
                   4, 0,
                   QVector<QTextLayout::FormatRange>()
                       << formatRange(26, 88))}
            << QString();

    QTest::newRow("instantiation with line:column info")
            << QString::fromLatin1("file.h: In function 'void UnitTest::CheckEqual(UnitTest::TestResults&, const Expected&, const Actual&, const UnitTest::TestDetails&) [with Expected = unsigned int, Actual = int]':\n"
                                   "file.cpp:87:10: instantiated from here\n"
                                   "file.h:21:5: warning: comparison between signed and unsigned integer expressions [-Wsign-compare]")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << compileTask(Task::Unknown,
                                "In function 'void UnitTest::CheckEqual(UnitTest::TestResults&, const Expected&, const Actual&, const UnitTest::TestDetails&) [with Expected = unsigned int, Actual = int]':\n"
                                "file.h: In function 'void UnitTest::CheckEqual(UnitTest::TestResults&, const Expected&, const Actual&, const UnitTest::TestDetails&) [with Expected = unsigned int, Actual = int]':\n"
                                "file.cpp:87:10: instantiated from here",
                                FilePath::fromUserInput("file.h"),
                                -1, 0,
                                QVector<QTextLayout::FormatRange>()
                                    << formatRange(172, 218))
                 << CompileTask(Task::Warning,
                                "comparison between signed and unsigned integer expressions [-Wsign-compare]",
                                FilePath::fromUserInput("file.h"),
                                21, 5))
            << QString();

    QTest::newRow("linker error") // QTCREATORBUG-3107
            << QString::fromLatin1("cns5k_ins_parser_tests.cpp:(.text._ZN20CNS5kINSParserEngine21DropBytesUntilStartedEP14CircularBufferIhE[CNS5kINSParserEngine::DropBytesUntilStarted(CircularBuffer<unsigned char>*)]+0x6d): undefined reference to `CNS5kINSPacket::SOH_BYTE'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `CNS5kINSPacket::SOH_BYTE'",
                               FilePath::fromUserInput("cns5k_ins_parser_tests.cpp")))
            << QString();

    QTest::newRow("libimf warning")
            << QString::fromLatin1("libimf.so: warning: warning: feupdateenv is not implemented and will always fail")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Warning,
                                "warning: feupdateenv is not implemented and will always fail",
                                FilePath::fromUserInput("libimf.so")))
            << QString();

    QTest::newRow("gcc 4.8")
            << QString::fromLatin1("In file included from /home/code/src/creator/src/libs/extensionsystem/pluginerrorview.cpp:31:0:\n"
                                   ".uic/ui_pluginerrorview.h:14:25: fatal error: QtGui/QAction: No such file or directory\n"
                                   " #include <QtGui/QAction>\n"
                                   "                         ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(
                   Task::Error,
                   "QtGui/QAction: No such file or directory\n"
                   "In file included from /home/code/src/creator/src/libs/extensionsystem/pluginerrorview.cpp:31:0:\n"
                   ".uic/ui_pluginerrorview.h:14:25: fatal error: QtGui/QAction: No such file or directory\n"
                   " #include <QtGui/QAction>\n"
                   "                         ^",
                   FilePath::fromUserInput(".uic/ui_pluginerrorview.h"),
                   14, 25,
                   QVector<QTextLayout::FormatRange>()
                       << formatRange(41, 22)
                       << formatRange(63, 67, "olpfile:///home/code/src/creator/src/libs/extensionsystem/pluginerrorview.cpp::31::0")
                       << formatRange(130, 146))}
            << QString();

    QTest::newRow("qtcreatorbug-9195")
            << QString::fromLatin1("In file included from /usr/include/qt4/QtCore/QString:1:0,\n"
                                   "                 from main.cpp:3:\n"
                                   "/usr/include/qt4/QtCore/qstring.h: In function 'void foo()':\n"
                                   "/usr/include/qt4/QtCore/qstring.h:597:5: error: 'QString::QString(const char*)' is private\n"
                                   "main.cpp:7:22: error: within this context")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(
                   Task::Error,
                   "'QString::QString(const char*)' is private\n"
                   "In file included from /usr/include/qt4/QtCore/QString:1:0,\n"
                   "                 from main.cpp:3:\n"
                   "/usr/include/qt4/QtCore/qstring.h: In function 'void foo()':\n"
                   "/usr/include/qt4/QtCore/qstring.h:597:5: error: 'QString::QString(const char*)' is private\n"
                   "main.cpp:7:22: error: within this context",
                   FilePath::fromUserInput("/usr/include/qt4/QtCore/qstring.h"),
                   597, 5,
                   QVector<QTextLayout::FormatRange>()
                       << formatRange(43, 22)
                       << formatRange(65, 31, "olpfile:///usr/include/qt4/QtCore/QString::1::0")
                       << formatRange(96, 40)
                       << formatRange(136, 33, "olpfile:///usr/include/qt4/QtCore/qstring.h::0::0")
                       << formatRange(169, 28)
                       << formatRange(197, 33, "olpfile:///usr/include/qt4/QtCore/qstring.h::597::5")
                       << formatRange(230, 99))}
            << QString();

    QTest::newRow("ld: Multiple definition error")
            << QString::fromLatin1("foo.o: In function `foo()':\n"
                                   "/home/user/test/foo.cpp:2: multiple definition of `foo()'\n"
                                   "bar.o:/home/user/test/bar.cpp:4: first defined here\n"
                                   "collect2: error: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << compileTask(Task::Error,
                               "multiple definition of `foo()'\n"
                               "foo.o: In function `foo()':\n"
                               "/home/user/test/foo.cpp:2: multiple definition of `foo()'",
                               FilePath::fromUserInput("/home/user/test/foo.cpp"),
                               2, 0,
                               QVector<QTextLayout::FormatRange>()
                                   << formatRange(31, 28)
                                   << formatRange(59, 23, "olpfile:///home/user/test/foo.cpp::2::-1")
                                   << formatRange(82, 34))
                << CompileTask(Task::Error,
                               "first defined here",
                               FilePath::fromUserInput("/home/user/test/bar.cpp"),
                               4)
                << CompileTask(Task::Error,
                               "collect2: error: ld returned 1 exit status"))
            << QString();

    QTest::newRow("ld: .data section")
            << QString::fromLatin1("foo.o:(.data+0x0): multiple definition of `foo'\n"
                                   "bar.o:(.data+0x0): first defined here\n"
                                   "collect2: error: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "multiple definition of `foo'",
                               FilePath::fromUserInput("foo.o"), -1)
                << CompileTask(Task::Error,
                               "first defined here",
                               FilePath::fromUserInput("bar.o"), -1)
                << CompileTask(Task::Error,
                               "collect2: error: ld returned 1 exit status"))
            << QString();


    QTest::newRow("Undefined symbol (Apple ld)")
            << "Undefined symbols for architecture x86_64:\n"
               "  \"SvgLayoutTest()\", referenced from:\n"
               "      _main in main.cpp.o"
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks({CompileTask(Task::Error,
                              "Undefined symbols for architecture x86_64:\n"
                              "Undefined symbols for architecture x86_64:\n"
                              "  \"SvgLayoutTest()\", referenced from:\n"
                              "      _main in main.cpp.o",
                              "main.cpp.o")})
            << QString();

    QTest::newRow("ld: undefined member function reference")
            << "obj/gtest-clang-printing.o:gtest-clang-printing.cpp:llvm::VerifyDisableABIBreakingChecks: error: undefined reference to 'llvm::DisableABIBreakingChecks'"
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "error: undefined reference to 'llvm::DisableABIBreakingChecks'",
                               "gtest-clang-printing.cpp"))
            << QString();

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
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks({
                   errorTask("ld.lld: error: undefined symbol: func()"),
                   unknownTask("referenced by test.cpp:5", "test.cpp", 5),
                   unknownTask("/tmp/ccg8pzRr.o:(main)",  "/tmp/ccg8pzRr.o"),
                   errorTask("collect2: error: ld returned 1 exit status")})
            << QString();

    QTest::newRow("lld: undefined reference with debug info (more verbose format)")
            << "ld.lld: error: undefined symbol: someFunc()\n"
               ">>> referenced by main.cpp:10 (/tmp/untitled4/main.cpp:10)\n"
               ">>>               /tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o:(main)\n"
               "clang-8: error: linker command failed with exit code 1 (use -v to see invocation)"
            << OutputParserTester::STDERR << QString()
            << QString("clang-8: error: linker command failed with exit code 1 (use -v to see invocation)\n")
            << Tasks{
                   errorTask("ld.lld: error: undefined symbol: someFunc()"),
                   unknownTask("referenced by main.cpp:10 (/tmp/untitled4/main.cpp:10)",
                               "/tmp/untitled4/main.cpp", 10),
                   unknownTask("/tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o:(main)",
                               "/tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o")}
            << QString();

    QTest::newRow("lld: undefined reference without debug info")
            << "ld.lld: error: undefined symbol: func()\n"
               ">>> referenced by test.cpp\n"
               ">>>               /tmp/ccvjyJph.o:(main)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks{
                   errorTask("ld.lld: error: undefined symbol: func()"),
                   unknownTask("referenced by test.cpp", "test.cpp"),
                   unknownTask("/tmp/ccvjyJph.o:(main)",  "/tmp/ccvjyJph.o"),
                   errorTask("collect2: error: ld returned 1 exit status")}
            << QString();

    if (HostOsInfo::isWindowsHost()) {
        QTest::newRow("lld: undefined reference with mingw")
                << "lld-link: error: undefined symbol: __Z4funcv\n"
                   ">>> referenced by C:\\Users\\orgads\\AppData\\Local\\Temp\\cccApKoz.o:(.text)\n"
                   "collect2.exe: error: ld returned 1 exit status"
                << OutputParserTester::STDERR << QString() << QString()
                << Tasks{
                       errorTask("lld-link: error: undefined symbol: __Z4funcv"),
                       unknownTask("referenced by C:\\Users\\orgads\\AppData\\Local\\Temp\\cccApKoz.o:(.text)",
                                   "C:/Users/orgads/AppData/Local/Temp/cccApKoz.o"),
                       errorTask("collect2.exe: error: ld returned 1 exit status")}
                << QString();
    }

    QTest::newRow("lld: multiple definitions with debug info")
            << "ld.lld: error: duplicate symbol: func()\n"
               ">>> defined at test1.cpp:1\n"
               ">>>            test1.o:(func())\n"
               ">>> defined at test1.cpp:1\n"
               ">>>            test1.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks{
                   errorTask("ld.lld: error: duplicate symbol: func()"),
                   unknownTask("defined at test1.cpp:1", "test1.cpp", 1),
                   unknownTask("test1.o:(func())",  "test1.o"),
                   unknownTask("defined at test1.cpp:1", "test1.cpp", 1),
                   unknownTask("test1.o:(.text+0x0)",  "test1.o"),
                   errorTask("collect2: error: ld returned 1 exit status")}
            << QString();

    QTest::newRow("lld: multiple definitions with debug info (more verbose format)")
            << "ld.lld: error: duplicate symbol: theFunc()\n"
               ">>> defined at file.cpp:1 (/tmp/untitled3/file.cpp:1)\n"
               ">>>            /tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/file.cpp.o:(theFunc())\n"
               ">>> defined at main.cpp:5 (/tmp/untitled3/main.cpp:5)\n"
               ">>>            /tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/main.cpp.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
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
                   errorTask("collect2: error: ld returned 1 exit status")}
            << QString();

    QTest::newRow("lld: multiple definitions without debug info")
            << "ld.lld: error: duplicate symbol: func()\n"
               ">>> defined at test1.cpp\n"
               ">>>            test1.o:(func())\n"
               ">>> defined at test1.cpp\n"
               ">>>            test1.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks{
                   errorTask("ld.lld: error: duplicate symbol: func()"),
                   unknownTask("defined at test1.cpp", "test1.cpp"),
                   unknownTask("test1.o:(func())",  "test1.o"),
                   unknownTask("defined at test1.cpp", "test1.cpp"),
                   unknownTask("test1.o:(.text+0x0)",  "test1.o"),
                   errorTask("collect2: error: ld returned 1 exit status")}
            << QString();

    if (HostOsInfo::isWindowsHost()) {
        QTest::newRow("lld: multiple definitions with mingw")
                << "lld-link: error: duplicate symbol: __Z4funcv in test1.o and in test2.o\n"
                   "collect2.exe: error: ld returned 1 exit status"
                << OutputParserTester::STDERR << QString() << QString()
                << Tasks{
                       errorTask("lld-link: error: duplicate symbol: __Z4funcv in test1.o and in test2.o"),
                       errorTask("collect2.exe: error: ld returned 1 exit status", {})}
                << QString();
    }

    QTest::newRow("Mac: ranlib warning")
            << QString::fromLatin1("ranlib: file: lib/libtest.a(Test0.cpp.o) has no symbols")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "file: lib/libtest.a(Test0.cpp.o) has no symbols"))
            << QString();

    QTest::newRow("Mac: ranlib warning2")
            << QString::fromLatin1("/path/to/XCode/and/ranlib: file: lib/libtest.a(Test0.cpp.o) has no symbols")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "file: lib/libtest.a(Test0.cpp.o) has no symbols"))
            << QString();

    QTest::newRow("GCC 9 output")
            << QString("In file included from /usr/include/qt/QtCore/qlocale.h:43,\n"
                       "                 from /usr/include/qt/QtCore/qtextstream.h:46,\n"
                       "                 from /qtc/src/shared/proparser/proitems.cpp:31:\n"
                       "/usr/include/qt/QtCore/qvariant.h: In constructor ‘QVariant::QVariant(QVariant&&)’:\n"
                       "/usr/include/qt/QtCore/qvariant.h:273:25: warning: implicitly-declared ‘constexpr QVariant::Private& QVariant::Private::operator=(const QVariant::Private&)’ is deprecated [-Wdeprecated-copy]\n"
                       "  273 |     { other.d = Private(); }\n"
                       "      |                         ^\n"
                       "/usr/include/qt/QtCore/qvariant.h:399:16: note: because ‘QVariant::Private’ has user-provided ‘QVariant::Private::Private(const QVariant::Private&)’\n"
                       "  399 |         inline Private(const Private &other) Q_DECL_NOTHROW\n"
                       "      |                      ^~~~~~~)\n"
                       "t.cc: In function ‘int test(const shape&, const shape&)’:\n"
                       "t.cc:15:4: error: no match for ‘operator+’ (operand types are ‘boxed_value<double>’ and ‘boxed_value<double>’)\n"
                       "  14 |   return (width(s1) * height(s1)\n"
                       "     |           ~~~~~~~~~~~~~~~~~~~~~~\n"
                       "     |                     |\n"
                       "     |                     boxed_value<[...]>\n"
                       "  15 |    + width(s2) * height(s2));\n"
                       "     |    ^ ~~~~~~~~~~~~~~~~~~~~~~\n"
                       "     |                |\n"
                       "     |                boxed_value<[...]>\n"
                       "incomplete.c:1:6: error: ‘string’ in namespace ‘std’ does not name a type\n"
                       "  1 | std::string test(void)\n"
                       "    |      ^~~~~~\n"
                       "incomplete.c:1:1: note: ‘std::string’ is defined in header ‘<string>’; did you forget to ‘#include <string>’?\n"
                       " +++ |+#include <string>\n"
                       "  1 | std::string test(void)\n"
                       "param-type-mismatch.c: In function ‘caller’:\n"
                       "param-type-mismatch.c:5:24: warning: passing argument 2 of ‘callee’ makes pointer from integer without a cast [-Wint-conversion]\n"
                       "  5 |   return callee(first, second, third);\n"
                       "    |                        ^~~~~~\n"
                       "    |                        |\n"
                       "    |                        int\n"
                       "param-type-mismatch.c:1:40: note: expected ‘const char *’ but argument is of type ‘int’\n"
                       "  1 | extern int callee(int one, const char *two, float three);\n"
                       "    |                            ~~~~~~~~~~~~^~~"
               )
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(Task::Warning,
                                 "implicitly-declared ‘constexpr QVariant::Private& QVariant::Private::operator=(const QVariant::Private&)’ is deprecated [-Wdeprecated-copy]\n"
                                 "In file included from /usr/include/qt/QtCore/qlocale.h:43,\n"
                                 "                 from /usr/include/qt/QtCore/qtextstream.h:46,\n"
                                 "                 from /qtc/src/shared/proparser/proitems.cpp:31:\n"
                                 "/usr/include/qt/QtCore/qvariant.h: In constructor ‘QVariant::QVariant(QVariant&&)’:\n"
                                 "/usr/include/qt/QtCore/qvariant.h:273:25: warning: implicitly-declared ‘constexpr QVariant::Private& QVariant::Private::operator=(const QVariant::Private&)’ is deprecated [-Wdeprecated-copy]\n"
                                 "  273 |     { other.d = Private(); }\n"
                                 "      |                         ^\n"
                                 "/usr/include/qt/QtCore/qvariant.h:399:16: note: because ‘QVariant::Private’ has user-provided ‘QVariant::Private::Private(const QVariant::Private&)’\n"
                                 "  399 |         inline Private(const Private &other) Q_DECL_NOTHROW\n"
                                 "      |                      ^~~~~~~)",
                                 FilePath::fromUserInput("/usr/include/qt/QtCore/qvariant.h"),
                                 273, 25,
                                 QVector<QTextLayout::FormatRange>()
                                     << formatRange(140, 22)
                                     << formatRange(162, 32, "olpfile:///usr/include/qt/QtCore/qlocale.h::43::0")
                                     << formatRange(194, 27)
                                     << formatRange(221, 36, "olpfile:///usr/include/qt/QtCore/qtextstream.h::46::0")
                                     << formatRange(257, 27)
                                     << formatRange(284, 38, "olpfile:///qtc/src/shared/proparser/proitems.cpp::31::0")
                                     << formatRange(322, 5)
                                     << formatRange(327, 33, "olpfile:///usr/include/qt/QtCore/qvariant.h::0::0")
                                     << formatRange(360, 51)
                                     << formatRange(411, 33, "olpfile:///usr/include/qt/QtCore/qvariant.h::273::25")
                                     << formatRange(444, 229)
                                     << formatRange(673, 33, "olpfile:///usr/include/qt/QtCore/qvariant.h::399::16")
                                     << formatRange(706, 221)),
                     compileTask(Task::Error,
                                 "no match for ‘operator+’ (operand types are ‘boxed_value<double>’ and ‘boxed_value<double>’)\n"
                                 "t.cc: In function ‘int test(const shape&, const shape&)’:\n"
                                 "t.cc:15:4: error: no match for ‘operator+’ (operand types are ‘boxed_value<double>’ and ‘boxed_value<double>’)\n"
                                 "  14 |   return (width(s1) * height(s1)\n"
                                 "     |           ~~~~~~~~~~~~~~~~~~~~~~\n"
                                 "     |                     |\n"
                                 "     |                     boxed_value<[...]>\n"
                                 "  15 |    + width(s2) * height(s2));\n"
                                 "     |    ^ ~~~~~~~~~~~~~~~~~~~~~~\n"
                                 "     |                |\n"
                                 "     |                boxed_value<[...]>",
                                 FilePath::fromUserInput("t.cc"),
                                 15, 4,
                                 QVector<QTextLayout::FormatRange>()
                                     << formatRange(93, 460)),
                      compileTask(Task::Error,
                                  "‘string’ in namespace ‘std’ does not name a type\n"
                                  "incomplete.c:1:6: error: ‘string’ in namespace ‘std’ does not name a type\n"
                                  "  1 | std::string test(void)\n"
                                  "    |      ^~~~~~\n"
                                  "incomplete.c:1:1: note: ‘std::string’ is defined in header ‘<string>’; did you forget to ‘#include <string>’?\n"
                                  " +++ |+#include <string>\n"
                                  "  1 | std::string test(void)",
                                  FilePath::fromUserInput("incomplete.c"),
                                  1, 6,
                                  QVector<QTextLayout::FormatRange>()
                                      << formatRange(49, 284)),
                      compileTask(Task::Warning,
                                  "passing argument 2 of ‘callee’ makes pointer from integer without a cast [-Wint-conversion]\n"
                                  "param-type-mismatch.c: In function ‘caller’:\n"
                                  "param-type-mismatch.c:5:24: warning: passing argument 2 of ‘callee’ makes pointer from integer without a cast [-Wint-conversion]\n"
                                  "  5 |   return callee(first, second, third);\n"
                                  "    |                        ^~~~~~\n"
                                  "    |                        |\n"
                                  "    |                        int\n"
                                  "param-type-mismatch.c:1:40: note: expected ‘const char *’ but argument is of type ‘int’\n"
                                  "  1 | extern int callee(int one, const char *two, float three);\n"
                                  "    |                            ~~~~~~~~~~~~^~~",
                                  FilePath::fromUserInput("param-type-mismatch.c"),
                                  5, 24,
                                  QVector<QTextLayout::FormatRange>()
                                      << formatRange(92, 519))}
            << QString();

    QTest::newRow(R"("inlined from")")
            << QString("In file included from smallstringvector.h:30,\n"
                       "                 from smallstringio.h:28,\n"
                       "                 from gtest-creator-printing.h:29,\n"
                       "                 from googletest.h:41,\n"
                       "                 from smallstring-test.cpp:26:\n"
                       "In member function ‘void Utils::BasicSmallString<Size>::append(Utils::SmallStringView) [with unsigned int Size = 31]’,\n"
                       "    inlined from ‘Utils::BasicSmallString<Size>& Utils::BasicSmallString<Size>::operator+=(Utils::SmallStringView) [with unsigned int Size = 31]’ at smallstring.h:471:15,\n"
                       "    inlined from ‘virtual void SmallString_AppendLongSmallStringToShortSmallString_Test::TestBody()’ at smallstring-test.cpp:850:63:\n"
                       "smallstring.h:465:21: warning: writing 1 byte into a region of size 0 [-Wstringop-overflow=]\n"
                       "  465 |         at(newSize) = 0;\n"
                       "      |         ~~~~~~~~~~~~^~~")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(Task::Warning,
                                 "writing 1 byte into a region of size 0 [-Wstringop-overflow=]\n"
                                 "In file included from smallstringvector.h:30,\n"
                                                        "                 from smallstringio.h:28,\n"
                                                        "                 from gtest-creator-printing.h:29,\n"
                                                        "                 from googletest.h:41,\n"
                                                        "                 from smallstring-test.cpp:26:\n"
                                                        "In member function ‘void Utils::BasicSmallString<Size>::append(Utils::SmallStringView) [with unsigned int Size = 31]’,\n"
                                                        "    inlined from ‘Utils::BasicSmallString<Size>& Utils::BasicSmallString<Size>::operator+=(Utils::SmallStringView) [with unsigned int Size = 31]’ at smallstring.h:471:15,\n"
                                                        "    inlined from ‘virtual void SmallString_AppendLongSmallStringToShortSmallString_Test::TestBody()’ at smallstring-test.cpp:850:63:\n"
                                                        "smallstring.h:465:21: warning: writing 1 byte into a region of size 0 [-Wstringop-overflow=]\n"
                                                        "  465 |         at(newSize) = 0;\n"
                                                        "      |         ~~~~~~~~~~~~^~~",
                                 FilePath::fromUserInput("smallstring.h"),
                                 465, 21,
                                 QVector<QTextLayout::FormatRange>()
                                     << formatRange(62, 805))}
            << QString();

    QTest::newRow(R"("required from")")
            << QString(
                   "In file included from qmap.h:1,\n"
                   "                 from qvariant.h:47,\n"
                   "                 from ilocatorfilter.h:33,\n"
                   "                 from helpindexfilter.h:28,\n"
                   "                 from moc_helpindexfilter.cpp:10:\n"
                   "qmap.h: In instantiation of ‘struct QMapNode<QString, QUrl>’:\n"
                   "qmap.h:606:30:   required from ‘QMap<K, V>::QMap(const QMap<K, V>&) [with Key = QString; T = QUrl]’\n"
                   "qmap.h:1090:7:   required from ‘static constexpr void (* QtPrivate::QMetaTypeForType<S>::getCopyCtr())(const QtPrivate::QMetaTypeInterface*, void*, const void*) [with T = QMultiMap<QString, QUrl>; S = QMultiMap<QString, QUrl>; QtPrivate::QMetaTypeInterface::CopyCtrFn = void (*)(const QtPrivate::QMetaTypeInterface*, void*, const void*)]’\n"
                   "qmetatype.h:2765:32:   required from ‘QtPrivate::QMetaTypeInterface QtPrivate::QMetaTypeForType<QMultiMap<QString, QUrl> >::metaType’\n"
                   "qmetatype.h:2865:16:   required from ‘constexpr QtPrivate::QMetaTypeInterface* QtPrivate::qTryMetaTypeInterfaceForType() [with Unique = qt_meta_stringdata_Help__Internal__HelpIndexFilter_t; T = const QMultiMap<QString, QUrl>&]’\n"
                   "qmetatype.h:2884:55:   required from ‘QtPrivate::QMetaTypeInterface* const qt_incomplete_metaTypeArray [4]<qt_meta_stringdata_Help__Internal__HelpIndexFilter_t, void, const QMultiMap<QString, QUrl>&, const QString&, QStringList>’\n"
                   "moc_helpindexfilter.cpp:105:1:   required from here\n"
                   "qmap.h:110:7: error: ‘QMapNode<Key, T>::value’ has incomplete type\n"
                   "  110 |     T value;\n"
                   "      |       ^~~~~")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(Task::Error,
                                 "‘QMapNode<Key, T>::value’ has incomplete type\n"
                                 "In file included from qmap.h:1,\n"
                                 "                 from qvariant.h:47,\n"
                                 "                 from ilocatorfilter.h:33,\n"
                                 "                 from helpindexfilter.h:28,\n"
                                 "                 from moc_helpindexfilter.cpp:10:\n"
                                 "qmap.h: In instantiation of ‘struct QMapNode<QString, QUrl>’:\n"
                                 "qmap.h:606:30:   required from ‘QMap<K, V>::QMap(const QMap<K, V>&) [with Key = QString; T = QUrl]’\n"
                                 "qmap.h:1090:7:   required from ‘static constexpr void (* QtPrivate::QMetaTypeForType<S>::getCopyCtr())(const QtPrivate::QMetaTypeInterface*, void*, const void*) [with T = QMultiMap<QString, QUrl>; S = QMultiMap<QString, QUrl>; QtPrivate::QMetaTypeInterface::CopyCtrFn = void (*)(const QtPrivate::QMetaTypeInterface*, void*, const void*)]’\n"
                                 "qmetatype.h:2765:32:   required from ‘QtPrivate::QMetaTypeInterface QtPrivate::QMetaTypeForType<QMultiMap<QString, QUrl> >::metaType’\n"
                                 "qmetatype.h:2865:16:   required from ‘constexpr QtPrivate::QMetaTypeInterface* QtPrivate::qTryMetaTypeInterfaceForType() [with Unique = qt_meta_stringdata_Help__Internal__HelpIndexFilter_t; T = const QMultiMap<QString, QUrl>&]’\n"
                                 "qmetatype.h:2884:55:   required from ‘QtPrivate::QMetaTypeInterface* const qt_incomplete_metaTypeArray [4]<qt_meta_stringdata_Help__Internal__HelpIndexFilter_t, void, const QMultiMap<QString, QUrl>&, const QString&, QStringList>’\n"
                                 "moc_helpindexfilter.cpp:105:1:   required from here\n"
                                 "qmap.h:110:7: error: ‘QMapNode<Key, T>::value’ has incomplete type\n"
                                 "  110 |     T value;\n"
                                 "      |       ^~~~~",
                                 FilePath::fromUserInput("moc_helpindexfilter.cpp"),
                                 105, 1,
                                 QVector<QTextLayout::FormatRange>()
                                     << formatRange(46, 1458))}
            << QString();

    QTest::newRow(R"("requested here")")
            << QString(
                   "In file included from tst_addresscache.cpp:21:\n"
                   "In file included from /usr/include/qt/QtTest/QTest:1:\n"
                   "In file included from /usr/include/qt/QtTest/qtest.h:45:\n"
                   "/usr/include/qt/QtTest/qtestcase.h:449:34: warning: comparison of integers of different signs: 'const unsigned long long' and 'const int' [-Wsign-compare]\n"
                   "        return compare_helper(t1 == t2, \"Compared values are not the same\",\n"
                   "                              ~~ ^  ~~\n"
                   "tst_addresscache.cpp:79:13: note: in instantiation of function template specialization 'QTest::qCompare<unsigned long long, int>' requested here\n"
                   "            QCOMPARE(cached.offset, 0x100);\n"
                   "            ^\n"
                   "/usr/include/qt/QtTest/qtestcase.h:88:17: note: expanded from macro 'QCOMPARE'\n"
                   "    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))\\\n"
                   "                ^\n"
                   "1 warning generated.")
            << OutputParserTester::STDERR
            << QString() << QString("1 warning generated.\n")
            << Tasks{compileTask(Task::Warning,
                                 "comparison of integers of different signs: 'const unsigned long long' and 'const int' [-Wsign-compare]\n"
                                 "In file included from tst_addresscache.cpp:21:\n"
                                 "In file included from /usr/include/qt/QtTest/QTest:1:\n"
                                 "In file included from /usr/include/qt/QtTest/qtest.h:45:\n"
                                 "/usr/include/qt/QtTest/qtestcase.h:449:34: warning: comparison of integers of different signs: 'const unsigned long long' and 'const int' [-Wsign-compare]\n"
                                 "        return compare_helper(t1 == t2, \"Compared values are not the same\",\n"
                                 "                              ~~ ^  ~~\n"
                                 "tst_addresscache.cpp:79:13: note: in instantiation of function template specialization 'QTest::qCompare<unsigned long long, int>' requested here\n"
                                 "            QCOMPARE(cached.offset, 0x100);\n"
                                 "            ^\n"
                                 "/usr/include/qt/QtTest/qtestcase.h:88:17: note: expanded from macro 'QCOMPARE'\n"
                                 "    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))\\\n"
                                 "                ^",
                                 FilePath::fromUserInput("tst_addresscache.cpp"), 79, 13, {})}
            << QString();

    QTest::newRow(R"("note: here")")
            << QString(
                   "In file included from qmlprofilerstatisticsmodel.h:31,\n"
                   "                 from qmlprofilerstatisticsmodel.cpp:26:\n"
                   "qmlprofilerstatisticsmodel.cpp: In member function ‘virtual QVariant QmlProfiler::QmlProfilerStatisticsModel::data(const QModelIndex&, int) const’:\n"
                   "qtcassert.h:43:34: warning: this statement may fall through [-Wimplicit-fallthrough=]\n"
                   "   43 | #define QTC_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)\n"
                   "      |                                  ^~\n"
                   "qtcassert.h:43:34: note: in definition of macro ‘QTC_ASSERT’\n"
                   "   43 | #define QTC_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)\n"
                   "      |                                  ^~\n"
                   "qmlprofilerstatisticsmodel.cpp:365:5: note: here\n"
                   "  365 |     default:\n"
                   "      |     ^~~~~~~")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{compileTask(Task::Warning,
                                 "this statement may fall through [-Wimplicit-fallthrough=]\n"
                                 "In file included from qmlprofilerstatisticsmodel.h:31,\n"
                                 "                 from qmlprofilerstatisticsmodel.cpp:26:\n"
                                 "qmlprofilerstatisticsmodel.cpp: In member function ‘virtual QVariant QmlProfiler::QmlProfilerStatisticsModel::data(const QModelIndex&, int) const’:\n"
                                 "qtcassert.h:43:34: warning: this statement may fall through [-Wimplicit-fallthrough=]\n"
                                 "   43 | #define QTC_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)\n"
                                 "      |                                  ^~\n"
                                 "qtcassert.h:43:34: note: in definition of macro ‘QTC_ASSERT’\n"
                                 "   43 | #define QTC_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)\n"
                                 "      |                                  ^~\n"
                                 "qmlprofilerstatisticsmodel.cpp:365:5: note: here\n"
                                 "  365 |     default:\n"
                                 "      |     ^~~~~~~",
                                 FilePath::fromUserInput("qmlprofilerstatisticsmodel.cpp"), 365, 5, {})}
            << QString();

    QTest::newRow("cc1plus")
            << QString(
                   "cc1plus: error: one or more PCH files were found, but they were invalid\n"
                   "cc1plus: error: use -Winvalid-pch for more information\n"
                   "cc1plus: fatal error: .pch/Qt6Core5Compat: No such file or directory\n"
                   "cc1plus: warning: -Wformat-security ignored without -Wformat [-Wformat-security]\n"
                   "compilation terminated.")
            << OutputParserTester::STDERR
            << QString() << QString("compilation terminated.\n")
            << Tasks{
                   CompileTask(Task::Error, "one or more PCH files were found, but they were invalid"),
                   CompileTask(Task::Error, "use -Winvalid-pch for more information"),
                   CompileTask(Task::Error, ".pch/Qt6Core5Compat: No such file or directory", ".pch/Qt6Core5Compat"),
                   CompileTask(Task::Warning, "-Wformat-security ignored without -Wformat [-Wformat-security]")}
            << QString();

    QTest::newRow("clean path")
            << QString("/home/tim/path/to/sources/./and/more.h:15:22: error: blubb")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(Task::Error, "blubb", "/home/tim/path/to/sources/and/more.h",
                                 15, 22)}
            << QString();

    QTest::newRow("no line number")
        << QString::fromUtf8("In file included from /data/dev/creator/src/libs/utils/aspects.cpp:12:\n"
                             "/data/dev/creator/src/libs/utils/layoutbuilder.h: In instantiation of ‘Layouting::BuilderItem<X, XInterface>::I::I(const Inner&) [with Inner = Utils::BaseAspect; X = Layouting::Row; XInterface = Layouting::LayoutInterface]’:\n"
                             "/data/dev/creator/src/libs/utils/aspects.cpp:3454:13:   required from here\n"
                             "/data/dev/creator/src/libs/utils/layoutbuilder.h:79:51: error: use of deleted function ‘Utils::BaseAspect::BaseAspect(const Utils::BaseAspect&)’\n"
                             "   79 |             apply = [p](XInterface *x) { doit_nest(x, p); };\n"
                             "      |                                          ~~~~~~~~^~~~~")
        << OutputParserTester::STDERR
        << QString() << QString()
        << (Tasks{compileTask(
               Task::Error,
               "use of deleted function ‘Utils::BaseAspect::BaseAspect(const Utils::BaseAspect&)’\n"
               "In file included from /data/dev/creator/src/libs/utils/aspects.cpp:12:\n"
               "/data/dev/creator/src/libs/utils/layoutbuilder.h: In instantiation of ‘Layouting::BuilderItem<X, XInterface>::I::I(const Inner&) [with Inner = Utils::BaseAspect; X = Layouting::Row; XInterface = Layouting::LayoutInterface]’:\n"
               "/data/dev/creator/src/libs/utils/aspects.cpp:3454:13:   required from here\n"
               "/data/dev/creator/src/libs/utils/layoutbuilder.h:79:51: error: use of deleted function ‘Utils::BaseAspect::BaseAspect(const Utils::BaseAspect&)’\n"
               "   79 |             apply = [p](XInterface *x) { doit_nest(x, p); };\n"
               "      |                                          ~~~~~~~~^~~~~",
               FilePath::fromUserInput("/data/dev/creator/src/libs/utils/aspects.cpp"), 3454, 13,
               QVector<QTextLayout::FormatRange>{
                   formatRange(82, 22),
                   formatRange(104, 44, "olpfile:///data/dev/creator/src/libs/utils/aspects.cpp::12::0"),
                   formatRange(148, 5),
                   formatRange(153, 48, "olpfile:///data/dev/creator/src/libs/utils/layoutbuilder.h::0::0"),
                   formatRange(201, 177),
                   formatRange(378, 44, "olpfile:///data/dev/creator/src/libs/utils/aspects.cpp::3454::13"),
                   formatRange(422, 31),
                   formatRange(453, 48, "olpfile:///data/dev/creator/src/libs/utils/layoutbuilder.h::79::51"),
                   formatRange(501, 228)})})
        << QString();
}

void ProjectExplorerTest::testGccOutputParsers()
{
    OutputParserTester testbench;
    testbench.setLineParsers(GccParser::gccParserSuite());
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(Tasks, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}

} // ProjectExplorer::Internal

#endif // WITH_TESTS
