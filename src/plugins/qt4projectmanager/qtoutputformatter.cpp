/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qtoutputformatter.h"

#include <texteditor/basetexteditor.h>
#include <qt4projectmanager/qt4project.h>

#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtGui/QPlainTextEdit>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

QtOutputFormatter::QtOutputFormatter(Qt4Project *project)
    : OutputFormatter()
    , m_qmlError(QLatin1String("(file:///.+:\\d+:\\d+):"))
    , m_qtError(QLatin1String("Object::.*in (.*:\\d+)"))
    , m_qtAssert(QLatin1String("^ASSERT: .* in file (.+, line \\d+)$"))
    , m_project(project)
{
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
    }
    return lr;
}

void QtOutputFormatter::appendApplicationOutput(const QString &txt, bool onStdErr)
{
    // Do the initialization lazily, as we don't have a plaintext edit
    // in the ctor
    if (!m_linkFormat.isValid()) {
        m_linkFormat.setForeground(plainTextEdit()->palette().link().color());
        m_linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        m_linkFormat.setAnchor(true);
    }

    QString text = txt;
    text.remove(QLatin1Char('\r'));

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
                clearLastLine();
                appendLine(lr, line, onStdErr);
            } else {
                // Found nothing, just emit the new part
                append(newPart, onStdErr ? StdErrFormat : StdOutFormat);
            }
            // Handled line continuation
            m_lastLine.clear();
        } else {
            const QString line = txt.mid(start, pos - start + 1);
            LinkResult lr = matchLine(line);
            if (!lr.href.isEmpty()) {
                appendLine(lr, line, onStdErr);
            } else {
                append(line, onStdErr ? StdErrFormat : StdOutFormat);
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
                clearLastLine();
                appendLine(lr, m_lastLine, onStdErr);
            } else {
                // Found nothing, just emit the new part
                append(newPart, onStdErr ? StdErrFormat : StdOutFormat);
            }
        } else {
            m_lastLine = txt.mid(start);
            LinkResult lr = matchLine(m_lastLine);
            if (!lr.href.isEmpty()) {
                appendLine(lr, m_lastLine, onStdErr);
            } else {
                append(m_lastLine, onStdErr ? StdErrFormat : StdOutFormat);
            }
        }
    }
}

void QtOutputFormatter::appendLine(LinkResult lr, const QString &line, bool onStdErr)
{
    append(line.left(lr.start), onStdErr ? StdErrFormat : StdOutFormat);
    m_linkFormat.setAnchorHref(lr.href);
    append(line.mid(lr.start, lr.end - lr.start), m_linkFormat);
    append(line.mid(lr.end), onStdErr ? StdErrFormat : StdOutFormat);
}

void QtOutputFormatter::handleLink(const QString &href)
{
    if (!href.isEmpty()) {
        const QRegExp qmlErrorLink(QLatin1String("^(file:///.+):(\\d+):(\\d+)"));

        if (qmlErrorLink.indexIn(href) != -1) {
            const QString fileName = QUrl(qmlErrorLink.cap(1)).toLocalFile();
            const int line = qmlErrorLink.cap(2).toInt();
            const int column = qmlErrorLink.cap(3).toInt();
            TextEditor::BaseTextEditor::openEditorAt(fileName, line, column - 1);
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

        if (!fileName.isEmpty()) {
            QFileInfo fi(fileName);
            if (fi.isRelative()) {
                // Yeah fileName is relative, no suprise
                Qt4Project *pro = m_project.data();
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
            }
            TextEditor::BaseTextEditor::openEditorAt(fileName, line, 0);
            return;
        }
    }
}
