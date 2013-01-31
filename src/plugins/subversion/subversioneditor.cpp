/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "subversioneditor.h"
#include "subversionplugin.h"

#include "annotationhighlighter.h"
#include "subversionconstants.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QDebug>
#include <QFileInfo>
#include <QTextCursor>
#include <QTextBlock>

using namespace Subversion;
using namespace Subversion::Internal;

SubversionEditor::SubversionEditor(const VcsBase::VcsBaseEditorParameters *type,
                                   QWidget *parent) :
    VcsBase::VcsBaseEditorWidget(type, parent),
    m_changeNumberPattern(QLatin1String("^\\d+$")),
    m_revisionNumberPattern(QLatin1String("^r\\d+$"))
{
    QTC_ASSERT(m_changeNumberPattern.isValid(), return);
    QTC_ASSERT(m_revisionNumberPattern.isValid(), return);
    /* Diff pattern:
    \code
        Index: main.cpp
    ===================================================================
    --- main.cpp<tab>(revision 2)
    +++ main.cpp<tab>(working copy)
    @@ -6,6 +6,5 @@
    \endcode
    */
    setDiffFilePattern(QRegExp(QLatin1String("^[-+]{3} ([^\\t]+)|^Index: .*|^=+$")));
    setLogEntryPattern(QRegExp(QLatin1String("^(r\\d+) \\|")));
    setAnnotateRevisionTextFormat(tr("Annotate revision \"%1\""));
}

QSet<QString> SubversionEditor::annotationChanges() const
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
    if (Subversion::Constants::debug)
        qDebug() << "SubversionEditor::annotationChanges() returns #" << changes.size();
    return changes;
}

QString SubversionEditor::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    QString change = cursor.selectedText();
    // Annotation output has number, log output has revision numbers
    // as r1, r2...
    if (m_changeNumberPattern.exactMatch(change))
        return change;
    if (m_revisionNumberPattern.exactMatch(change)) {
        change.remove(0, 1);
        return change;
    }
    return QString();
}

VcsBase::BaseAnnotationHighlighter *SubversionEditor::createAnnotationHighlighter(const QSet<QString> &changes,
                                                                                  const QColor &bg) const
{
    return new SubversionAnnotationHighlighter(changes, bg);
}

QStringList SubversionEditor::annotationPreviousVersions(const QString &v) const
{
    bool ok;
    const int revision = v.toInt(&ok);
    if (!ok || revision < 2)
        return QStringList();
    return QStringList(QString::number(revision - 1));
}
