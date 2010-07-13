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
#include <QtGui/QPlainTextEdit>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

QtOutputFormatter::QtOutputFormatter(Qt4Project *project)
    : OutputFormatter()
    , m_qmlError(QLatin1String("(file:///[^:]+:\\d+:\\d+):"))
    , m_qtError(QLatin1String("Object::.*in (.*:\\d+)"))
    , m_project(project)

{

}

void QtOutputFormatter::appendApplicationOutput(const QString &text, bool onStdErr)
{
    QTextCharFormat linkFormat;
    linkFormat.setForeground(plainTextEdit()->palette().link().color());
    linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    linkFormat.setAnchor(true);

    // Create links from QML errors (anything of the form "file:///...:[line]:[column]:")
    if (m_qmlError.indexIn(text) != -1) {
        const int matchPos = m_qmlError.pos(1);
        const QString leader = text.left(matchPos);
        append(leader, onStdErr ? StdErrFormat : StdOutFormat);

        const QString matched = m_qmlError.cap(1);
        linkFormat.setAnchorHref(matched);
        append(matched, linkFormat);

        int index = matchPos + m_qmlError.matchedLength() - 1;
        append(text.mid(index), onStdErr ? StdErrFormat : StdOutFormat);
    } else if (m_qtError.indexIn(text) != -1) {
        const int matchPos = m_qtError.pos(1);
        const QString leader = text.left(matchPos);
        append(leader, onStdErr ? StdErrFormat : StdOutFormat);

        const QString matched = m_qtError.cap(1);
        linkFormat.setAnchorHref(m_qtError.cap(1));
        append(matched, linkFormat);

        int index = matchPos + m_qtError.matchedLength() - 1;
        append(text.mid(index), onStdErr ? StdErrFormat : StdOutFormat);
    }
}

void QtOutputFormatter::handleLink(const QString &href)
{
    if (!href.isEmpty()) {
        QRegExp qmlErrorLink(QLatin1String("^file://(/[^:]+):(\\d+):(\\d+)"));

        if (qmlErrorLink.indexIn(href) != -1) {
            const QString fileName = qmlErrorLink.cap(1);
            const int line = qmlErrorLink.cap(2).toInt();
            const int column = qmlErrorLink.cap(3).toInt();
            TextEditor::BaseTextEditor::openEditorAt(fileName, line, column - 1);
            return;
        }

        QRegExp qtErrorLink(QLatin1String("^(.*):(\\d+)$"));
        if (qtErrorLink.indexIn(href) != 1) {
            QString fileName = qtErrorLink.cap(1);
            const int line = qtErrorLink.cap(2).toInt();
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
