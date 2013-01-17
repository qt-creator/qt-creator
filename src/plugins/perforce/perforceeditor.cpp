/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "perforceeditor.h"

#include "annotationhighlighter.h"
#include "perforceconstants.h"
#include "perforceplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QRegExp>
#include <QSet>
#include <QTextStream>

#include <QAction>
#include <QKeyEvent>
#include <QLayout>
#include <QMenu>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextBlock>

namespace Perforce {
namespace Internal {

// ------------ PerforceEditor
PerforceEditor::PerforceEditor(const VcsBase::VcsBaseEditorParameters *type,
                               QWidget *parent)  :
    VcsBase::VcsBaseEditorWidget(type, parent),
    m_changeNumberPattern(QLatin1String("^\\d+$")),
    m_plugin(PerforcePlugin::perforcePluginInstance())
{
    QTC_CHECK(m_changeNumberPattern.isValid());
    setAnnotateRevisionTextFormat(tr("Annotate change list \"%1\""));
    if (Perforce::Constants::debug)
        qDebug() << "PerforceEditor::PerforceEditor" << type->type << type->id;
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

QRegExp PerforceEditor::diffFilePattern() const
{
    return QRegExp(QLatin1String("^====.*"));
}

VcsBase::BaseAnnotationHighlighter *PerforceEditor::createAnnotationHighlighter(const QSet<QString> &changes,
                                                                                const QColor &bg) const
{
    return new PerforceAnnotationHighlighter(changes, bg);
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
            const QString fileName = m_plugin->fileNameFromPerforceName(diffFileName.trimmed(), false, &errorMessage);
            if (fileName.isEmpty())
                qWarning("%s", qPrintable(errorMessage));
            return fileName;
        }
    }
    return QString();
}

QStringList PerforceEditor::annotationPreviousVersions(const QString &v) const
{
    bool ok;
    const int changeList = v.toInt(&ok);
    if (!ok || changeList < 2)
        return QStringList();
    return QStringList(QString::number(changeList - 1));
}

} // namespace Internal
} // namespace Perforce
