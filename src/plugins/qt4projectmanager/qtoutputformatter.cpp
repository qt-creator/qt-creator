/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qtoutputformatter.h"

#include <texteditor/basetexteditor.h>
#include <qt4projectmanager/qt4project.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextCursor>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

QtOutputFormatter::QtOutputFormatter(ProjectExplorer::Project *project)
    : OutputFormatter()
    , m_qmlError(QLatin1String("^(file:///.+"    // file url
                               ":\\d+"           // colon, line
                               "(?::\\d+)?)"     // colon, column (optional)
                               ":"))             // colon
    , m_qtError(QLatin1String("Object::.*in (.*:\\d+)"))
    , m_qtAssert(QLatin1String("^ASSERT: .* in file (.+, line \\d+)$"))
    , m_qtTestFail(QLatin1String("^   Loc: \\[(.*)\\]$"))
    , m_project(project)
{
    if(project)
        m_projectFinder.setProjectDirectory(project->projectDirectory());
}

LinkResult QtOutputFormatter::matchLine(const QString &line) const
{
    LinkResult lr;
    lr.start = -1;
    lr.end = -1;

    if (m_qmlError.indexIn(line) != -1) {
        lr.href = m_qmlError.cap(1);
        lr.start = m_qmlError.pos(1);
        lr.end = lr.start + lr.href.length();
    } else if (m_qtError.indexIn(line) != -1) {
        lr.href = m_qtError.cap(1);
        lr.start = m_qtError.pos(1);
        lr.end = lr.start + lr.href.length();
    } else if (m_qtAssert.indexIn(line) != -1) {
        lr.href = m_qtAssert.cap(1);
        lr.start = m_qtAssert.pos(1);
        lr.end = lr.start + lr.href.length();
    } else if (m_qtTestFail.indexIn(line) != -1) {
        lr.href = m_qtTestFail.cap(1);
        lr.start = m_qtTestFail.pos(1);
        lr.end = lr.start + lr.href.length();
    }
    return lr;
}

void QtOutputFormatter::appendMessage(const QString &txt, OutputFormat format)
{
    QTextCursor cursor(plainTextEdit()->document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();

    QString text = txt;
    text.remove(QLatin1Char('\r'));

    QString deferedText;

    int start = 0;
    int pos = txt.indexOf(QLatin1Char('\n'));
    while (pos != -1) {
        // Line identified
        if (!m_lastLine.isEmpty()) {
            // Line continuation
            const QString newPart = txt.mid(start, pos - start + 1);
            const QString line = m_lastLine + newPart;
            LinkResult lr = matchLine(line);
            if (!lr.href.isEmpty()) {
                // Found something && line continuation
                cursor.insertText(deferedText, charFormat(format));
                deferedText.clear();
                clearLastLine();
                appendLine(cursor, lr, line, format);
            } else {
                // Found nothing, just emit the new part
                deferedText += newPart;
            }
            // Handled line continuation
            m_lastLine.clear();
        } else {
            const QString line = txt.mid(start, pos - start + 1);
            LinkResult lr = matchLine(line);
            if (!lr.href.isEmpty()) {
                cursor.insertText(deferedText, charFormat(format));
                deferedText.clear();
                appendLine(cursor, lr, line, format);
            } else {
                deferedText += line;
            }
        }
        start = pos + 1;
        pos = txt.indexOf(QLatin1Char('\n'), start);
    }

    // Handle left over stuff
    if (start < txt.length()) {
        if (!m_lastLine.isEmpty()) {
            // Line continuation
            const QString newPart = txt.mid(start);
            m_lastLine.append(newPart);
            LinkResult lr = matchLine(m_lastLine);
            if (!lr.href.isEmpty()) {
                // Found something && line continuation
                cursor.insertText(deferedText, charFormat(format));
                deferedText.clear();
                clearLastLine();
                appendLine(cursor, lr, m_lastLine, format);
            } else {
                // Found nothing, just emit the new part
                deferedText += newPart;
            }
        } else {
            m_lastLine = txt.mid(start);
            LinkResult lr = matchLine(m_lastLine);
            if (!lr.href.isEmpty()) {
                cursor.insertText(deferedText, charFormat(format));
                deferedText.clear();
                appendLine(cursor, lr, m_lastLine, format);
            } else {
                deferedText += m_lastLine;
            }
        }
    }
    cursor.insertText(deferedText, charFormat(format));
    // deferedText.clear();
    cursor.endEditBlock();
}

void QtOutputFormatter::appendLine(QTextCursor &cursor, LinkResult lr,
    const QString &line, ProjectExplorer::OutputFormat format)
{
    const QTextCharFormat normalFormat = charFormat(format);
    cursor.insertText(line.left(lr.start), normalFormat);

    QTextCharFormat linkFormat = normalFormat;
    const QColor textColor = plainTextEdit()->palette().color(QPalette::Text);
    linkFormat.setForeground(mixColors(textColor, QColor(Qt::blue)));
    linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    linkFormat.setAnchor(true);
    linkFormat.setAnchorHref(lr.href);
    cursor.insertText(line.mid(lr.start, lr.end - lr.start), linkFormat);
    cursor.insertText(line.mid(lr.end), normalFormat);
}

void QtOutputFormatter::handleLink(const QString &href)
{
    if (!href.isEmpty()) {
        const QRegExp qmlLineColumnLink(QLatin1String("^(file:///.+)" // file url
                                                 ":(\\d+)"            // line
                                                 ":(\\d+)$"));        // column

        if (qmlLineColumnLink.indexIn(href) != -1) {
            const QString fileName = QUrl(qmlLineColumnLink.cap(1)).toLocalFile();
            const int line = qmlLineColumnLink.cap(2).toInt();
            const int column = qmlLineColumnLink.cap(3).toInt();
            TextEditor::BaseTextEditor::openEditorAt(m_projectFinder.findFile(fileName), line, column - 1);

            return;
        }

        const QRegExp qmlLineLink(QLatin1String("^(file:///.+)" // file url
                                                 ":(\\d+)$"));  // line

        if (qmlLineLink.indexIn(href) != -1) {
            const QString fileName = QUrl(qmlLineLink.cap(1)).toLocalFile();
            const int line = qmlLineLink.cap(2).toInt();
            TextEditor::BaseTextEditor::openEditorAt(m_projectFinder.findFile(fileName), line);
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
            QFileInfo fi(fileName);
            if (fi.isRelative()) {
                // Yeah fileName is relative, no surprise
                ProjectExplorer::Project *pro = m_project.data();
                if (pro) {
                    QString baseName = fi.fileName();
                    foreach (const QString &file, pro->files(Project::AllFiles)) {
                        if (file.endsWith(baseName)) {
                            // pick the first one...
                            fileName = file;
                            break;
                        }
                    }
                }
            } else if (!fi.exists()) {
                // map possible on-device path to source path
                fileName = m_projectFinder.findFile(fileName);
            }
            TextEditor::BaseTextEditor::openEditorAt(fileName, line, 0);
            return;
        }
    }
}
