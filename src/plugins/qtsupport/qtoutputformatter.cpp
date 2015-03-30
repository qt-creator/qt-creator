/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qtoutputformatter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <utils/ansiescapecodehandler.h>
#include <utils/fileinprojectfinder.h>
#include <utils/theme/theme.h>

#include <QPlainTextEdit>
#include <QPointer>
#include <QRegExp>
#include <QTextCursor>
#include <QUrl>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {

// "file" or "qrc", colon, optional '//', '/' and further characters
#define QML_URL_REGEXP \
    "(?:file|qrc):(?://)?/.+"

namespace Internal {

class QtOutputFormatterPrivate
{
public:
    QtOutputFormatterPrivate(Project *proj)
        : qmlError(QLatin1String("^(" QML_URL_REGEXP    // url
                                   ":\\d+"           // colon, line
                                   "(?::\\d+)?)"     // colon, column (optional)
                                   "[: \t]"))        // colon, space or tab
        , qtError(QLatin1String("Object::.*in (.*:\\d+)"))
        , qtAssert(QLatin1String("ASSERT: .* in file (.+, line \\d+)"))
        , qtAssertX(QLatin1String("ASSERT failure in .*: \".*\", file (.+, line \\d+)"))
        , qtTestFail(QLatin1String("^   Loc: \\[(.*)\\]"))
        , project(proj)
    {
    }

    ~QtOutputFormatterPrivate()
    {
    }

    QRegExp qmlError;
    QRegExp qtError;
    QRegExp qtAssert;
    QRegExp qtAssertX;
    QRegExp qtTestFail;
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
        d->projectFinder.setProjectFiles(project->files(Project::ExcludeGeneratedFiles));
        d->projectFinder.setProjectDirectory(project->projectDirectory().toString());

        connect(project, SIGNAL(fileListChanged()),
                this, SLOT(updateProjectFileList()));
    }
}

QtOutputFormatter::~QtOutputFormatter()
{
    delete d;
}

LinkResult QtOutputFormatter::matchLine(const QString &line) const
{
    LinkResult lr;
    lr.start = -1;
    lr.end = -1;

    if (d->qmlError.indexIn(line) != -1) {
        lr.href = d->qmlError.cap(1);
        lr.start = d->qmlError.pos(1);
        lr.end = lr.start + lr.href.length();
    } else if (d->qtError.indexIn(line) != -1) {
        lr.href = d->qtError.cap(1);
        lr.start = d->qtError.pos(1);
        lr.end = lr.start + lr.href.length();
    } else if (d->qtAssert.indexIn(line) != -1) {
        lr.href = d->qtAssert.cap(1);
        lr.start = d->qtAssert.pos(1);
        lr.end = lr.start + lr.href.length();
    } else if (d->qtAssertX.indexIn(line) != -1) {
        lr.href = d->qtAssertX.cap(1);
        lr.start = d->qtAssertX.pos(1);
        lr.end = lr.start + lr.href.length();
    } else if (d->qtTestFail.indexIn(line) != -1) {
        lr.href = d->qtTestFail.cap(1);
        lr.start = d->qtTestFail.pos(1);
        lr.end = lr.start + lr.href.length();
    }
    return lr;
}

void QtOutputFormatter::appendMessage(const QString &txt, OutputFormat format)
{
    appendMessage(txt, charFormat(format));
}

void QtOutputFormatter::appendMessagePart(QTextCursor &cursor, const QString &txt,
                                          const QTextCharFormat &format)
{
    QString deferredText;

    const int length = txt.length();
    for (int start = 0, pos = -1; start < length; start = pos + 1) {
        pos = txt.indexOf(QLatin1Char('\n'), start);
        const QString newPart = txt.mid(start, (pos == -1) ? -1 : pos - start + 1);
        const QString line = d->lastLine + newPart;

        LinkResult lr = matchLine(line);
        if (!lr.href.isEmpty()) {
            // Found something && line continuation
            cursor.insertText(deferredText, format);
            deferredText.clear();
            if (!d->lastLine.isEmpty())
                clearLastLine();
            appendLine(cursor, lr, line, format);
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
    cursor.insertText(deferredText, format);
}

void QtOutputFormatter::appendMessage(const QString &txt, const QTextCharFormat &format)
{
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);
    d->cursor.beginEditBlock();

    foreach (const FormattedText &output, parseAnsi(txt, format))
        appendMessagePart(d->cursor, output.text, output.format);

    d->cursor.endEditBlock();
}

void QtOutputFormatter::appendLine(QTextCursor &cursor, const LinkResult &lr,
                                   const QString &line, OutputFormat format)
{
    appendLine(cursor, lr, line, charFormat(format));
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

void QtOutputFormatter::appendLine(QTextCursor &cursor, const LinkResult &lr,
                                   const QString &line, const QTextCharFormat &format)
{
    cursor.insertText(line.left(lr.start), format);
    cursor.insertText(line.mid(lr.start, lr.end - lr.start), linkFormat(format, lr.href));
    cursor.insertText(line.mid(lr.end), format);
}

void QtOutputFormatter::handleLink(const QString &href)
{
    if (!href.isEmpty()) {
        QRegExp qmlLineColumnLink(QLatin1String("^(" QML_URL_REGEXP ")" // url
                                                ":(\\d+)"               // line
                                                ":(\\d+)$"));           // column

        if (qmlLineColumnLink.indexIn(href) != -1) {
            const QUrl fileUrl = QUrl(qmlLineColumnLink.cap(1));
            const int line = qmlLineColumnLink.cap(2).toInt();
            const int column = qmlLineColumnLink.cap(3).toInt();

            openEditor(d->projectFinder.findFile(fileUrl), line, column - 1);

            return;
        }

        QRegExp qmlLineLink(QLatin1String("^(" QML_URL_REGEXP ")" // url
                                          ":(\\d+)$"));  // line

        if (qmlLineLink.indexIn(href) != -1) {
            const QUrl fileUrl = QUrl(qmlLineLink.cap(1));
            const int line = qmlLineLink.cap(2).toInt();
            openEditor(d->projectFinder.findFile(d->projectFinder.findFile(fileUrl)), line);
            return;
        }

        QString fileName;
        int line = -1;

        QRegExp qtErrorLink(QLatin1String("^(.*):(\\d+)$"));
        if (qtErrorLink.indexIn(href) != -1) {
            fileName = qtErrorLink.cap(1);
            line = qtErrorLink.cap(2).toInt();
        }

        QRegExp qtAssertLink(QLatin1String("^(.+), line (\\d+)$"));
        if (qtAssertLink.indexIn(href) != -1) {
            fileName = qtAssertLink.cap(1);
            line = qtAssertLink.cap(2).toInt();
        }

        QRegExp qtTestFailLink(QLatin1String("^(.*)\\((\\d+)\\)$"));
        if (qtTestFailLink.indexIn(href) != -1) {
            fileName = qtTestFailLink.cap(1);
            line = qtTestFailLink.cap(2).toInt();
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
        d->projectFinder.setProjectFiles(d->project.data()->files(Project::ExcludeGeneratedFiles));
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
        QtOutputFormatter(0),
        line(-1),
        column(-1)
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
    int line;
    int column;
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
            << QString::fromLatin1("Pass through plain text.")
            << -1 << -1 << QString()
            << QString() << -1 << -1;

    QTest::newRow("qrc:/main.qml:20")
            << QString::fromLatin1("qrc:/main.qml:20 Unexpected token `identifier'")
            << 0 << 16 << QString::fromLatin1("qrc:/main.qml:20")
            << QString::fromLatin1("/main.qml") << 20 << -1;

    QTest::newRow("qrc:///main.qml:20")
            << QString::fromLatin1("qrc:///main.qml:20 Unexpected token `identifier'")
            << 0 << 18 << QString::fromLatin1("qrc:///main.qml:20")
            << QString::fromLatin1("/main.qml") << 20 << -1;

    QTest::newRow("file:///main.qml:20")
            << QString::fromLatin1("file:///main.qml:20 Unexpected token `identifier'")
            << 0 << 19 << QString::fromLatin1("file:///main.qml:20")
            << QString::fromLatin1("/main.qml") << 20 << -1;
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
            << QString::fromLatin1("test\n123")
            << QString::fromLatin1("test\n123")
            << QTextCharFormat()
            << QTextCharFormat();
    QTest::newRow("Qt error")
            << QString::fromLatin1("Object::Test in test.cpp:123")
            << QString::fromLatin1("Object::Test in test.cpp:123")
            << QTextCharFormat()
            << linkFormat(QTextCharFormat(), QLatin1String("test.cpp:123"));
    QTest::newRow("colored")
            << QString::fromLatin1("blue da ba dee")
            << QString::fromLatin1("blue da ba dee")
            << blueFormat()
            << blueFormat();
    QTest::newRow("ANSI color change")
            << QString::fromLatin1("\x1b[38;2;0;0;127mHello")
            << QString::fromLatin1("Hello")
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

    const QString inputText = QString::fromLatin1(
                "\x1b[38;2;0;0;127mHello\n"
                "Object::Test in test.cpp:123\n"
                "\x1b[38;2;0;0;127mHello\n");
    const QString outputText = QString::fromLatin1(
                "Hello\n"
                "Object::Test in test.cpp:123\n"
                "Hello\n");

    formatter.appendMessage(inputText, QTextCharFormat());

    QCOMPARE(edit.toPlainText(), outputText);

    edit.moveCursor(QTextCursor::Start);
    QCOMPARE(edit.currentCharFormat(), blueFormat());

    edit.moveCursor(QTextCursor::Down);
    edit.moveCursor(QTextCursor::EndOfLine);
    QCOMPARE(edit.currentCharFormat(), linkFormat(QTextCharFormat(), QLatin1String("test.cpp:123")));

    edit.moveCursor(QTextCursor::End);
    QCOMPARE(edit.currentCharFormat(), blueFormat());
}

} // namespace QtSupport

#endif // WITH_TESTS
