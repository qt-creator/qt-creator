// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "outputparsers.h"

#include "ioutputparser.h"
#include "projectexplorertr.h"
#include "task.h"

#include <utils/hostosinfo.h>

#include <QRegularExpression>

#ifdef WITH_TESTS
#include <utils/algorithm.h>
#include <QPlainTextEdit>
#include <QTest>
#endif

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class OsParser : public OutputTaskParser
{
public:
    OsParser() { setObjectName(QLatin1String("OsParser")); }

private:
    Result handleLine(const QString &line, OutputFormat type) override
    {
        if (type == StdOutFormat) {
            if (HostOsInfo::isWindowsHost()) {
                const QString trimmed = line.trimmed();
                if (trimmed
                    == QLatin1String(
                        "The process cannot access the file because it is "
                        "being used by another process.")) {
                    scheduleTask(
                        CompileTask(
                            Task::Error,
                            Tr::tr(
                                "The process cannot access the file because it is being used "
                                "by another process.\n"
                                "Please close all running instances of your application before "
                                "starting a build.")),
                        1);
                    m_hasFatalError = true;
                    return Status::Done;
                }
            }
            return Status::NotHandled;
        }
        if (HostOsInfo::isLinuxHost()) {
            const QString trimmed = line.trimmed();
            if (trimmed.contains(QLatin1String(": error while loading shared libraries:"))) {
                scheduleTask(CompileTask(Task::Error, trimmed), 1);
                return Status::Done;
            }
        }
        return Status::NotHandled;
    }

    bool hasFatalErrors() const override { return m_hasFatalError; }

    bool m_hasFatalError = false;
};

class GenericOutputParser : public OutputLineParser
{
public:

private:
    Result handleLine(const QString &line, OutputFormat format) override
    {
        if (format != OutputFormat::StdOutFormat && format != OutputFormat::StdErrFormat)
            return Status::NotHandled;

        static const QRegularExpression regex(pattern());

        LinkSpecs linkSpecs;
        qsizetype offset = 0;
        while (true) {
            const QRegularExpressionMatch match = regex.match(line, offset);
            if (!match.hasMatch())
                break;
            linkSpecs.emplaceBack(match.capturedStart(), match.captured().size(), match.captured());
            offset = match.capturedEnd();
        }

        if (linkSpecs.isEmpty())
            return Status::NotHandled;

        return {Status::Done, linkSpecs};
    }


    static QString pattern()
    {
        const QString possibleDrive("(?:[A-Za-z]:)?");
        const QString lineOrColumnSuffix("(?::\\d+)");

        // A path with "weird characters" can only be properly detected if it is followed
        // by a line number.
        const QString unlimitedPath
            = QString("(?:[^:]+%1%1?)").arg(lineOrColumnSuffix);
        const QString limitedPath = QString("(?:[A-Za-z0-9./\\\\_-]+%1?%1?)").arg(lineOrColumnSuffix);
        const QString path = QString("(?:%1|%2)").arg(unlimitedPath, limitedPath);
        return QString("(file://%1%2)").arg(possibleDrive, path);
    }
};

#ifdef WITH_TESTS
class GenericOutputParserTest : public QObject
{
    Q_OBJECT
private slots:
    void test_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<OutputLineParser::LinkSpecs>("linkSpecs");

        QTest::addRow("no match") << QString("This only looks a bit like a fil:///prefix")
                                  << OutputLineParser::LinkSpecs();
        QTest::addRow("UNIX path with line and column")
            << QString("the path: file:///main.cpp:2:3")
            << OutputLineParser::LinkSpecs{{10, 20, "file:///main.cpp:2:3"}};
        QTest::addRow("relative UNIX path with line")
            << QString("the path: file://file with spaces.cpp:2")
            << OutputLineParser::LinkSpecs{{10, 29, "file://file with spaces.cpp:2"}};
        QTest::addRow("UNIX path with no line")
            << QString("the path: file:///main.cpp")
            << OutputLineParser::LinkSpecs{{10, 16, "file:///main.cpp"}};
        QTest::addRow("UNIX path with spaces and no line")
            << QString("Cannot be properly detected: file:///file with spaces.cpp")
            << OutputLineParser::LinkSpecs{{29, 12, "file:///file"}};
        QTest::addRow("Windows path with line and column")
            << QString("the path: file://C:\\main.cpp:2:3")
            << OutputLineParser::LinkSpecs{{10, 23, "file://C:\\main.cpp:2:3"}};
        QTest::addRow("Windows path with line")
            << QString("the path: file://D:\\file with spaces.cpp:2")
            << OutputLineParser::LinkSpecs{{10, 33, "file://D:\\file with spaces.cpp:2"}};
        QTest::addRow("relative Windows path with no line")
            << QString("the path: file://C:main.cpp")
            << OutputLineParser::LinkSpecs{{10, 17, "file://C:main.cpp"}};
        QTest::addRow("Windows path with spaces and no line")
            << QString("Cannot be properly detected: file://C:\\file with spaces.cpp")
            << OutputLineParser::LinkSpecs{{29, 14, "file://C:\\file"}};
        QTest::addRow("More than one file on a line")
            << QString(
                   "A plain file:///file1.cpp, then a file:///file2.cpp:1199 with a line number, "
                   "then\none on a different line with line and column: file:///file3.cpp:42:7")
            << OutputLineParser::LinkSpecs{{8, 17, "file:///file1.cpp"},
                                           {34, 22, "file:///file2.cpp:1199"},
                                           {128, 22, "file:///file3.cpp:42:7"}};
    }

    void test()
    {
        QFETCH(QString, input);
        QFETCH(OutputLineParser::LinkSpecs, linkSpecs);

        for (OutputFormat format : {OutputFormat::StdOutFormat, OutputFormat::StdErrFormat}) {
            QPlainTextEdit edit;
            OutputFormatter formatter;
            formatter.setPlainTextEdit(&edit);
            formatter.setLineParsers({new GenericOutputParser});
            formatter.appendMessage(input, format);
            formatter.flush();

            QCOMPARE(edit.toPlainText(), input);

            const QTextCharFormat baseFormat = formatter.charFormat(format);
            QTextCursor c(edit.document());
            for (const OutputLineParser::LinkSpec &linkSpec : std::as_const(linkSpecs)) {
                const QTextCharFormat expectedLinkFormat
                    = OutputFormatter::linkFormat(baseFormat, linkSpec.target);

                // The indices are "off by one" because QTextCursor::charFormat() returns the
                // format before the current position.
                const int lastLinkPos = std::min(
                    linkSpec.startPos + linkSpec.length + 1, edit.document()->characterCount() - 1);
                for (int i = linkSpec.startPos + 1; i < lastLinkPos; ++i) {
                    c.setPosition(i);
                    if (c.charFormat() != expectedLinkFormat)
                        qDebug() << i << c.document()->characterAt(i) << linkSpec.target;
                    QCOMPARE(c.charFormat(), expectedLinkFormat);
                }
            }
            for (int i = 0; i < edit.document()->characterCount(); ++i) {
                if (!Utils::contains(linkSpecs, [i](const OutputLineParser::LinkSpec &linkSpec) {
                        return i > linkSpec.startPos && i <= linkSpec.startPos + linkSpec.length;
                    })) {
                    c.setPosition(i);
                    if (c.charFormat() != baseFormat)
                        qDebug() << i << c.document()->characterAt(i);
                    QCOMPARE(c.charFormat(), baseFormat);
                }
            }
        }
    }
};

QObject *createGenericOutputParserTest()
{
    return new GenericOutputParserTest;
}
#endif

} // namespace Internal

Utils::OutputLineParser *createOsOutputParser()
{
    return new Internal::OsParser;
}

OutputLineParser *createGenericOutputParser()
{
    return new Internal::GenericOutputParser;
}

} // namespace ProjectExplorer

#ifdef WITH_TESTS
#include <outputparsers.moc>
#endif
