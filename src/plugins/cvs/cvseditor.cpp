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

#include "cvseditor.h"
#include "cvsutils.h"

#include "annotationhighlighter.h"
#include "cvsconstants.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QDebug>
#include <QTextCursor>
#include <QTextBlock>

namespace Cvs {
namespace Internal {

// Match a CVS revision ("1.1.1.1")
#define CVS_REVISION_PATTERN "[\\d\\.]+"
#define CVS_REVISION_AT_START_PATTERN "^(" CVS_REVISION_PATTERN ") "

CvsEditor::CvsEditor(const VcsBase::VcsBaseEditorParameters *type,
                                   QWidget *parent) :
    VcsBase::VcsBaseEditorWidget(type, parent),
    m_revisionAnnotationPattern(QLatin1String(CVS_REVISION_AT_START_PATTERN ".*$")),
    m_revisionLogPattern(QLatin1String("^revision  *(" CVS_REVISION_PATTERN ")$"))
{
    QTC_ASSERT(m_revisionAnnotationPattern.isValid(), return);
    QTC_ASSERT(m_revisionLogPattern.isValid(), return);
    /* Diff format:
    \code
    cvs diff -d -u -r1.1 -r1.2:
    --- mainwindow.cpp<\t>13 Jul 2009 13:50:15 -0000<tab>1.1
    +++ mainwindow.cpp<\t>14 Jul 2009 07:09:24 -0000<tab>1.2
    @@ -6,6 +6,5 @@
    \endcode
    */
    setDiffFilePattern(QRegExp(QLatin1String("^[-+]{3} ([^\\t]+)")));
    setLogEntryPattern(QRegExp(QLatin1String("^revision (.+)$")));
    setAnnotateRevisionTextFormat(tr("Annotate revision \"%1\""));
}

QSet<QString> CvsEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // Hunt for first change number in annotation: "1.1 (author)"
    QRegExp r(QLatin1String(CVS_REVISION_AT_START_PATTERN));
    QTC_ASSERT(r.isValid(), return changes);
    if (r.indexIn(txt) != -1) {
        changes.insert(r.cap(1));
        r.setPattern(QLatin1String("\n(" CVS_REVISION_PATTERN ") "));
        QTC_ASSERT(r.isValid(), return changes);
        int pos = 0;
        while ((pos = r.indexIn(txt, pos)) != -1) {
            pos += r.matchedLength();
            changes.insert(r.cap(1));
        }
    }
    if (Cvs::Constants::debug)
        qDebug() << "CVSEditor::annotationChanges() returns #" << changes.size();
    return changes;
}

QString CvsEditor::changeUnderCursor(const QTextCursor &c) const
{
    // Try to match "1.1" strictly:
    // 1) Annotation: Check for a revision number at the beginning of the line.
    //    Note that "cursor.select(QTextCursor::WordUnderCursor)" will
    //    only select the part up until the dot.
    //    Check if we are at the beginning of a line within a reasonable offset.
    // 2) Log: check for lines like "revision 1.1", cursor past "revision"
    switch (contentType()) {
    case VcsBase::RegularCommandOutput:
    case VcsBase::DiffOutput:
        break;
    case VcsBase::AnnotateOutput: {
            const QTextBlock block = c.block();
            if (c.atBlockStart() || (c.position() - block.position() < 3)) {
                const QString line = block.text();
                if (m_revisionAnnotationPattern.exactMatch(line))
                    return m_revisionAnnotationPattern.cap(1);
            }
        }
        break;
    case VcsBase::LogOutput: {
            const QTextBlock block = c.block();
            if (c.position() - block.position() > 8 && m_revisionLogPattern.exactMatch(block.text()))
                return m_revisionLogPattern.cap(1);
        }
        break;
    }
    return QString();
}

VcsBase::BaseAnnotationHighlighter *CvsEditor::createAnnotationHighlighter(const QSet<QString> &changes,
                                                                           const QColor &bg) const
{
    return new CvsAnnotationHighlighter(changes, bg);
}

QStringList CvsEditor::annotationPreviousVersions(const QString &revision) const
{
    if (isFirstRevision(revision))
        return QStringList();
    return QStringList(previousRevision(revision));
}

}
}
