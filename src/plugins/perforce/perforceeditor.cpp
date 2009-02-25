/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "perforceeditor.h"

#include "annotationhighlighter.h"
#include "perforceconstants.h"
#include "perforceplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QKeyEvent>
#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>

namespace Perforce {
namespace Internal {

// ------------ PerforceEditor
PerforceEditor::PerforceEditor(const VCSBase::VCSBaseEditorParameters *type,
                               QWidget *parent)  :
    VCSBase::VCSBaseEditor(type, parent),
    m_changeNumberPattern(QLatin1String("^\\d+$")),
    m_plugin(PerforcePlugin::perforcePluginInstance())
{
    QTC_ASSERT(m_changeNumberPattern.isValid(), /**/);
    if (Perforce::Constants::debug)
        qDebug() << "PerforceEditor::PerforceEditor" << type->type << type->kind;
}

QSet<QString> PerforceEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // Hunt for first change number in annotation: "<change>:"
    QRegExp r(QLatin1String("^(\\d+):"));
    QTC_ASSERT(r.isValid(), return changes);
    if (r.indexIn(txt) != -1) {
        changes.insert(r.cap(1));
        r.setPattern(QLatin1String("\n(\\d+):"));
        QTC_ASSERT(r.isValid(), return changes);
        int pos = 0;
        while ((pos = r.indexIn(txt, pos)) != -1) {
            pos += r.matchedLength();
            changes.insert(r.cap(1));
        }
    }
    if (Perforce::Constants::debug)
        qDebug() << "PerforceEditor::annotationChanges() returns #" << changes.size();
    return changes;
}

QString PerforceEditor::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    const QString change = cursor.selectedText();
    return m_changeNumberPattern.exactMatch(change) ? change : QString();
}

VCSBase::DiffHighlighter *PerforceEditor::createDiffHighlighter() const
{
    const QRegExp filePattern(QLatin1String("^====.*"));
    return new VCSBase::DiffHighlighter(filePattern);
}

VCSBase::BaseAnnotationHighlighter *PerforceEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new PerforceAnnotationHighlighter(changes);
}

QString PerforceEditor::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
    QString errorMessage;
    const QString diffIndicator = QLatin1String("==== ");
    const QString diffEndIndicator = QLatin1String(" ====");
    // Go back chunks. Note that for 'describe', an extra, empty line
    // occurs.
    for (QTextBlock  block = inBlock; block.isValid(); block = block.previous()) {
        QString diffFileName = block.text();
        if (diffFileName.startsWith(diffIndicator) && diffFileName.endsWith(diffEndIndicator)) {
            // Split:
            // 1) "==== //depot/.../mainwindow.cpp#2 - /depot/.../mainwindow.cpp ===="
            // (as created by p4 diff) or
            // 2) "==== //depot/.../mainwindow.cpp#15 (text) ===="
            // (as created by p4 describe).
            diffFileName.remove(0, diffIndicator.size());
            diffFileName.truncate(diffFileName.size() - diffEndIndicator.size());
            const int separatorPos = diffFileName.indexOf(QLatin1String(" - "));
            if (separatorPos == -1) {
                // ==== depot path (text) ==== (p4 describe)
                const int blankPos = diffFileName.indexOf(QLatin1Char(' '));
                if (blankPos == -1)
                    return QString();
                diffFileName.truncate(blankPos);
            } else {
                // ==== depot path - local path ==== (p4 diff)
                diffFileName.truncate(separatorPos);
            }
            // Split off revision "#4"
            const int revisionPos = diffFileName.lastIndexOf(QLatin1Char('#'));
            if (revisionPos != -1 && revisionPos < diffFileName.length() - 1)
                diffFileName.truncate(revisionPos);
            // Ask plugin to map back
            const QString fileName = m_plugin->fileNameFromPerforceName(diffFileName.trimmed(), &errorMessage);
            if (fileName.isEmpty())
                qWarning("%s", qPrintable(errorMessage));
            return fileName;
        }
    }
    return QString();
}

} // namespace Internal
} // namespace Perforce
