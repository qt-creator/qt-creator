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

#include "perforceeditor.h"

#include "annotationhighlighter.h"
#include "perforceplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/diffandloghighlighter.h>

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
PerforceEditorWidget::PerforceEditorWidget() :
    m_changeNumberPattern(QLatin1String("^\\d+$"))
{
    QTC_CHECK(m_changeNumberPattern.isValid());
    // Diff format:
    // 1) "==== //depot/.../mainwindow.cpp#2 - /depot/.../mainwindow.cpp ====" (created by p4 diff)
    // 2) "==== //depot/.../mainwindow.cpp#15 (text) ====" (created by p4 describe)
    // 3) --- //depot/XXX/closingkit/trunk/source/cui/src/cui_core.cpp<tab>2012-02-08 13:54:01.000000000 0100
    //    +++ P:/XXX\closingkit\trunk\source\cui\src\cui_core.cpp<tab>2012-02-08 13:54:01.000000000 0100
    setDiffFilePattern(QRegExp(QLatin1String("^(?:={4}|\\+{3}) (.+)(?:\\t|#\\d)")));
    setLogEntryPattern(QRegExp(QLatin1String("^... #\\d change (\\d+) ")));
    setAnnotateRevisionTextFormat(tr("Annotate change list \"%1\""));
}

QSet<QString> PerforceEditorWidget::annotationChanges() const
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
    return changes;
}

QString PerforceEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    const QString change = cursor.selectedText();
    return m_changeNumberPattern.exactMatch(change) ? change : QString();
}

VcsBase::BaseAnnotationHighlighter *PerforceEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new PerforceAnnotationHighlighter(changes);
}

QString PerforceEditorWidget::findDiffFile(const QString &f) const
{
    QString errorMessage;
    const QString fileName = PerforcePlugin::fileNameFromPerforceName(f.trimmed(), false, &errorMessage);
    if (fileName.isEmpty())
        qWarning("%s", qPrintable(errorMessage));
    return fileName;
}

QStringList PerforceEditorWidget::annotationPreviousVersions(const QString &v) const
{
    bool ok;
    const int changeList = v.toInt(&ok);
    if (!ok || changeList < 2)
        return QStringList();
    return QStringList(QString::number(changeList - 1));
}

} // namespace Internal
} // namespace Perforce
