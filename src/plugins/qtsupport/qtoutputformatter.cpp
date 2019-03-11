/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qtoutputformatter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>

#include <utils/algorithm.h>
#include <utils/ansiescapecodehandler.h>
#include <utils/fileinprojectfinder.h>
#include <utils/hostosinfo.h>
#include <utils/theme/theme.h>

#include <QPlainTextEdit>
#include <QPointer>
#include <QRegularExpression>
#include <QTextCursor>
#include <QUrl>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {

namespace Internal {

class QtOutputFormatterPrivate
{
public:
    QtOutputFormatterPrivate(Project *proj)
        : qmlError("(" QT_QML_URL_REGEXP  // url
                   ":\\d+"              // colon, line
                   "(?::\\d+)?)"        // colon, column (optional)
                   "\\b")               // word boundary
        , qtError("Object::.*in (.*:\\d+)")
        , qtAssert(QT_ASSERT_REGEXP)
        , qtAssertX(QT_ASSERT_X_REGEXP)
        , qtTestFailUnix(QT_TEST_FAIL_UNIX_REGEXP)
        , qtTestFailWin(QT_TEST_FAIL_WIN_REGEXP)
        , project(proj)
    {
    }

    ~QtOutputFormatterPrivate()
    {
    }

    const QRegularExpression qmlError;
    const QRegularExpression qtError;
    const QRegularExpression qtAssert;
    const QRegularExpression qtAssertX;
    const QRegularExpression qtTestFailUnix;
    const QRegularExpression qtTestFailWin;
    QPointer<Project> project;
    QString lastLine;
    FileInProjectFinder projectFinder;
    QTextCursor cursor;
};

} // namespace Internal

QtOutputFormatter::QtOutputFormatter(Project *project)
    : d(new Internal::QtOutputFormatterPrivate(project))
{
    if (project) {
        d->projectFinder.setProjectFiles(project->files(Project::SourceFiles));
        d->projectFinder.setProjectDirectory(project->projectDirectory());

        connect(project, &Project::fileListChanged,
                this, &QtOutputFormatter::updateProjectFileList);
    }
}

QtOutputFormatter::~QtOutputFormatter()
{
    delete d;
}

LinkResult QtOutputFormatter::matchLine(const QString &line) const
{
    LinkResult lr;

    auto hasMatch = [&lr, line](const QRegularExpression &regex) {
        const QRegularExpressionMatch match = regex.match(line);
        if (!match.hasMatch())
            return false;

        lr.href = match.captured(1);
        lr.start = match.capturedStart(1);
        lr.end = lr.start + lr.href.length();
        return true;
    };

    if (hasMatch(d->qmlError))
        return lr;
    if (hasMatch(d->qtError))
        return lr;
    if (hasMatch(d->qtAssert))
        return lr;
    if (hasMatch(d->qtAssertX))
        return lr;
    if (hasMatch(d->qtTestFailUnix))
        return lr;
    if (hasMatch(d->qtTestFailWin))
        return lr;

    return lr;
}

void QtOutputFormatter::appendMessage(const QString &txt, OutputFormat format)
{
    appendMessage(txt, charFormat(format));
}

void QtOutputFormatter::appendMessagePart(const QString &txt, const QTextCharFormat &format)
{
    QString deferredText;

    const int length = txt.length();
    for (int start = 0, pos = -1; start < length; start = pos + 1) {
        pos = txt.indexOf('\n', start);
        const QString newPart = txt.mid(start, (pos == -1) ? -1 : pos - start + 1);
        const QString line = d->lastLine + newPart;

        LinkResult lr = matchLine(line);
        if (!lr.href.isEmpty()) {
            // Found something && line continuation
            d->cursor.insertText(deferredText, format);
            deferredText.clear();
            if (!d->lastLine.isEmpty())
                clearLastLine();
            appendLine(lr, line, format);
        } else {
            // Found nothing, just emit the new part
            deferredText += newPart;
        }

        if (pos == -1) {
            d->lastLine = line;
            break;
        }
        d->lastLine.clear(); // Handled line continuation
    }
    d->cursor.insertText(deferredText, format);
}

void QtOutputFormatter::appendMessage(const QString &txt, const QTextCharFormat &format)
{
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);
    d->cursor.beginEditBlock();

    const QList<FormattedText> ansiTextList = parseAnsi(txt, format);
    for (const FormattedText &output : ansiTextList)
        appendMessagePart(output.text, output.format);

    d->cursor.endEditBlock();
}

void QtOutputFormatter::appendLine(const LinkResult &lr, const QString &line, OutputFormat format)
{
    appendLine(lr, line, charFormat(format));
}

static QTextCharFormat linkFormat(const QTextCharFormat &inputFormat, const QString &href)
{
    QTextCharFormat result = inputFormat;
    result.setForeground(creatorTheme()->color(Theme::TextColorLink));
    result.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    result.setAnchor(true);
    result.setAnchorHref(href);

    return result;
}

void QtOutputFormatter::appendLine(const LinkResult &lr, const QString &line,
                                   const QTextCharFormat &format)
{
    d->cursor.insertText(line.left(lr.start), format);
    d->cursor.insertText(line.mid(lr.start, lr.end - lr.start), linkFormat(format, lr.href));
    d->cursor.insertText(line.mid(lr.end), format);
}

void QtOutputFormatter::handleLink(const QString &href)
{
    if (!href.isEmpty()) {
        static const QRegularExpression qmlLineColumnLink("^(" QT_QML_URL_REGEXP ")" // url
                                                          ":(\\d+)"                  // line
                                                          ":(\\d+)$");               // column
        const QRegularExpressionMatch qmlLineColumnMatch = qmlLineColumnLink.match(href);

        if (qmlLineColumnMatch.hasMatch()) {
            const QUrl fileUrl = QUrl(qmlLineColumnMatch.captured(1));
            const int line = qmlLineColumnMatch.captured(2).toInt();
            const int column = qmlLineColumnMatch.captured(3).toInt();

            openEditor(d->projectFinder.findFile(fileUrl), line, column - 1);

            return;
        }

        static const QRegularExpression qmlLineLink("^(" QT_QML_URL_REGEXP ")" // url
                                                    ":(\\d+)$");               // line
        const QRegularExpressionMatch qmlLineMatch = qmlLineLink.match(href);

        if (qmlLineMatch.hasMatch()) {
            const QUrl fileUrl = QUrl(qmlLineMatch.captured(1));
            const int line = qmlLineMatch.captured(2).toInt();
            openEditor(d->projectFinder.findFile(fileUrl), line);
            return;
        }

        QString fileName;
        int line = -1;

        static const QRegularExpression qtErrorLink("^(.*):(\\d+)$");
        const QRegularExpressionMatch qtErrorMatch = qtErrorLink.match(href);
        if (qtErrorMatch.hasMatch()) {
            fileName = qtErrorMatch.captured(1);
            line = qtErrorMatch.captured(2).toInt();
        }

        static const QRegularExpression qtAssertLink("^(.+), line (\\d+)$");
        const QRegularExpressionMatch qtAssertMatch = qtAssertLink.match(href);
        if (qtAssertMatch.hasMatch()) {
            fileName = qtAssertMatch.captured(1);
            line = qtAssertMatch.captured(2).toInt();
        }

        static const QRegularExpression qtTestFailLink("^(.*)\\((\\d+)\\)$");
        const QRegularExpressionMatch qtTestFailMatch = qtTestFailLink.match(href);
        if (qtTestFailMatch.hasMatch()) {
            fileName = qtTestFailMatch.captured(1);
            line = qtTestFailMatch.captured(2).toInt();
        }

        if (!fileName.isEmpty()) {
            fileName = d->projectFinder.findFile(QUrl::fromLocalFile(fileName));
            openEditor(fileName, line);
            return;
        }
    }
}

void QtOutputFormatter::setPlainTextEdit(QPlainTextEdit *plainText)
{
    OutputFormatter::setPlainTextEdit(plainText);
    d->cursor = plainText ? plainText->textCursor() : QTextCursor();
}

void QtOutputFormatter::clearLastLine()
{
    OutputFormatter::clearLastLine();
    d->lastLine.clear();
}

void QtOutputFormatter::openEditor(const QString &fileName, int line, int column)
{
    Core::EditorManager::openEditorAt(fileName, line, column);
}

void QtOutputFormatter::updateProjectFileList()
{
    if (d->project)
        d->projectFinder.setProjectFiles(d->project->files(Project::SourceFiles));
}

} // namespace QtSupport

// Unit tests:

#ifdef WITH_TESTS

#   include <QTest>

#   include "qtsupportplugin.h"

Q_DECLARE_METATYPE(QTextCharFormat)

namespace QtSupport {

using namespace QtSupport::Internal;

class TestQtOutputFormatter : public QtOutputFormatter
{
public:
    TestQtOutputFormatter() :
        QtOutputFormatter(nullptr)
    {
    }

    void openEditor(const QString &fileName, int line, int column = -1)
    {
        this->fileName = fileName;
        this->line = line;
        this->column = column;
    }

public:
    QString fileName;
    int line = -1;
    int column = -1;
};


void QtSupportPlugin::testQtOutputFormatter_data()
{
    QTest::addColumn<QString>("input");

    // matchLine results
    QTest::addColumn<int>("linkStart");
    QTest::addColumn<int>("linkEnd");
    QTest::addColumn<QString>("href");

    // handleLink results
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");

    QTest::newRow("pass through")
            << "Pass through plain text."
            << -1 << -1 << QString()
            << QString() << -1 << -1;

    QTest::newRow("qrc:/main.qml:20")
            << "qrc:/main.qml:20 Unexpected token `identifier'"
            << 0 << 16 << "qrc:/main.qml:20"
            << "/main.qml" << 20 << -1;

    QTest::newRow("qrc:///main.qml:20")
            << "qrc:///main.qml:20 Unexpected token `identifier'"
            << 0 << 18 << "qrc:///main.qml:20"
            << "/main.qml" << 20 << -1;

    QTest::newRow("onClicked (qrc:/main.qml:20)")
            << "onClicked (qrc:/main.qml:20)"
            << 11 << 27 << "qrc:/main.qml:20"
            << "/main.qml" << 20 << -1;

    QTest::newRow("file:///main.qml:20")
            << "file:///main.qml:20 Unexpected token `identifier'"
            << 0 << 19 << "file:///main.qml:20"
            << "/main.qml" << 20 << -1;

    QTest::newRow("File link without further text")
            << "file:///home/user/main.cpp:157"
            << 0 << 30 << "file:///home/user/main.cpp:157"
            << "/home/user/main.cpp" << 157 << -1;

    QTest::newRow("File link with text before")
            << "Text before: file:///home/user/main.cpp:157"
            << 13 << 43 << "file:///home/user/main.cpp:157"
            << "/home/user/main.cpp" << 157 << -1;

    QTest::newRow("File link with text afterwards")
            << "file:///home/user/main.cpp:157: Text afterwards"
            << 0 << 30 << "file:///home/user/main.cpp:157"
            << "/home/user/main.cpp" << 157 << -1;

    QTest::newRow("File link with text before and afterwards")
            << "Text before file:///home/user/main.cpp:157 and text afterwards"
            << 12 << 42 << "file:///home/user/main.cpp:157"
            << "/home/user/main.cpp" << 157 << -1;

    QTest::newRow("Unix file link with timestamp")
            << "file:///home/user/main.cpp:157 2018-03-21 10:54:45.706"
            << 0 << 30 << "file:///home/user/main.cpp:157"
            << "/home/user/main.cpp" << 157 << -1;

    QTest::newRow("Windows file link with timestamp")
            << "file:///e:/path/main.cpp:157 2018-03-21 10:54:45.706"
            << 0 << 28 << "file:///e:/path/main.cpp:157"
            << (Utils::HostOsInfo::isWindowsHost()
                ? "e:/path/main.cpp"
                : "/e:/path/main.cpp")
            << 157 << -1;

    QTest::newRow("Unix failed QTest link")
            << "   Loc: [../TestProject/test.cpp(123)]"
            << 9 << 37 << "../TestProject/test.cpp(123)"
            << "../TestProject/test.cpp" << 123 << -1;
    if (HostOsInfo::isWindowsHost()) {
        QTest::newRow("Windows failed QTest link")
                << "..\\TestProject\\test.cpp(123) : failure location"
                << 0 << 28 << "..\\TestProject\\test.cpp(123)"
                << "../TestProject/test.cpp" << 123 << -1;
        QTest::newRow("Windows failed QTest link with carriage return")
                << "..\\TestProject\\test.cpp(123) : failure location\r"
                << 0 << 28 << "..\\TestProject\\test.cpp(123)"
                << "../TestProject/test.cpp" << 123 << -1;
    }
}

void QtSupportPlugin::testQtOutputFormatter()
{
    QFETCH(QString, input);

    QFETCH(int, linkStart);
    QFETCH(int, linkEnd);
    QFETCH(QString, href);

    QFETCH(QString, file);
    QFETCH(int, line);
    QFETCH(int, column);

    TestQtOutputFormatter formatter;

    LinkResult result = formatter.matchLine(input);
    formatter.handleLink(result.href);

    QCOMPARE(result.start, linkStart);
    QCOMPARE(result.end, linkEnd);
    QCOMPARE(result.href, href);

    QCOMPARE(formatter.fileName, file);
    QCOMPARE(formatter.line, line);
    QCOMPARE(formatter.column, column);
}

static QTextCharFormat blueFormat()
{
    QTextCharFormat result;
    result.setForeground(QColor(0, 0, 127));
    return result;
}

void QtSupportPlugin::testQtOutputFormatter_appendMessage_data()
{
    QTest::addColumn<QString>("inputText");
    QTest::addColumn<QString>("outputText");
    QTest::addColumn<QTextCharFormat>("inputFormat");
    QTest::addColumn<QTextCharFormat>("outputFormat");

    QTest::newRow("pass through")
            << "test\n123"
            << "test\n123"
            << QTextCharFormat()
            << QTextCharFormat();
    QTest::newRow("Qt error")
            << "Object::Test in test.cpp:123"
            << "Object::Test in test.cpp:123"
            << QTextCharFormat()
            << linkFormat(QTextCharFormat(), "test.cpp:123");
    QTest::newRow("colored")
            << "blue da ba dee"
            << "blue da ba dee"
            << blueFormat()
            << blueFormat();
    QTest::newRow("ANSI color change")
            << "\x1b[38;2;0;0;127mHello"
            << "Hello"
            << QTextCharFormat()
            << blueFormat();
}

void QtSupportPlugin::testQtOutputFormatter_appendMessage()
{
    QPlainTextEdit edit;
    TestQtOutputFormatter formatter;
    formatter.setPlainTextEdit(&edit);

    QFETCH(QString, inputText);
    QFETCH(QString, outputText);
    QFETCH(QTextCharFormat, inputFormat);
    QFETCH(QTextCharFormat, outputFormat);

    formatter.appendMessage(inputText, inputFormat);

    QCOMPARE(edit.toPlainText(), outputText);
    QCOMPARE(edit.currentCharFormat(), outputFormat);
}

void QtSupportPlugin::testQtOutputFormatter_appendMixedAssertAndAnsi()
{
    QPlainTextEdit edit;
    TestQtOutputFormatter formatter;
    formatter.setPlainTextEdit(&edit);

    const QString inputText =
                "\x1b[38;2;0;0;127mHello\n"
                "Object::Test in test.cpp:123\n"
                "\x1b[38;2;0;0;127mHello\n";
    const QString outputText =
                "Hello\n"
                "Object::Test in test.cpp:123\n"
                "Hello\n";

    formatter.appendMessage(inputText, QTextCharFormat());

    QCOMPARE(edit.toPlainText(), outputText);

    edit.moveCursor(QTextCursor::Start);
    QCOMPARE(edit.currentCharFormat(), blueFormat());

    edit.moveCursor(QTextCursor::Down);
    edit.moveCursor(QTextCursor::EndOfLine);
    QCOMPARE(edit.currentCharFormat(), linkFormat(QTextCharFormat(), "test.cpp:123"));

    edit.moveCursor(QTextCursor::End);
    QCOMPARE(edit.currentCharFormat(), blueFormat());
}

} // namespace QtSupport

#endif // WITH_TESTS
