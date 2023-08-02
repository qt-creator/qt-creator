// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cvseditor.h"

#include "cvstr.h"
#include "cvsutils.h"

#include <utils/qtcassert.h>

#include <vcsbase/baseannotationhighlighter.h>
#include <vcsbase/diffandloghighlighter.h>

#include <QDebug>
#include <QTextCursor>
#include <QTextBlock>

namespace Cvs::Internal {

// Match a CVS revision ("1.1.1.1")
#define CVS_REVISION_PATTERN "[\\d\\.]+"
#define CVS_REVISION_AT_START_PATTERN "^(" CVS_REVISION_PATTERN ") "

// Annotation highlighter for cvs triggering on 'changenumber '
class CvsAnnotationHighlighter : public VcsBase::BaseAnnotationHighlighter
{
public:
    explicit CvsAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                      QTextDocument *document = nullptr) :
        VcsBase::BaseAnnotationHighlighter(changeNumbers, document)
    { }

private:
    QString changeNumber(const QString &block) const override
    {
        const int pos = block.indexOf(QLatin1Char(' '));
        return pos > 1 ? block.left(pos) : QString();
    }
};

CvsEditorWidget::CvsEditorWidget() :
    m_revisionAnnotationPattern(CVS_REVISION_AT_START_PATTERN),
    m_revisionLogPattern("^revision  *(" CVS_REVISION_PATTERN ")$")
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
    setDiffFilePattern("^[-+]{3} ([^\\t]+)");
    setLogEntryPattern("^revision (.+)$");
    setAnnotateRevisionTextFormat(Tr::tr("Annotate revision \"%1\""));
    setAnnotationEntryPattern("^(" CVS_REVISION_PATTERN ") ");
}

QString CvsEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    // Try to match "1.1" strictly:
    // 1) Annotation: Check for a revision number at the beginning of the line.
    //    Note that "cursor.select(QTextCursor::WordUnderCursor)" will
    //    only select the part up until the dot.
    //    Check if we are at the beginning of a line within a reasonable offset.
    // 2) Log: check for lines like "revision 1.1", cursor past "revision"
    switch (contentType()) {
    case VcsBase::OtherContent:
    case VcsBase::DiffOutput:
        break;
    case VcsBase::AnnotateOutput: {
            const QTextBlock block = c.block();
            if (c.atBlockStart() || (c.position() - block.position() < 3)) {
                const QString line = block.text();
                const QRegularExpressionMatch match = m_revisionAnnotationPattern.match(line);
                if (match.hasMatch())
                    return match.captured(1);
            }
        }
        break;
    case VcsBase::LogOutput: {
            const QTextBlock block = c.block();
            if (c.position() - block.position() > 8) {
                const QRegularExpressionMatch match = m_revisionLogPattern.match(block.text());
                if (match.hasMatch())
                    return match.captured(1);
            }
        }
        break;
    }
    return {};
}

VcsBase::BaseAnnotationHighlighter *CvsEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new CvsAnnotationHighlighter(changes);
}

QStringList CvsEditorWidget::annotationPreviousVersions(const QString &revision) const
{
    if (isFirstRevision(revision))
        return {};
    return QStringList(previousRevision(revision));
}

} // Cvs::Internal
