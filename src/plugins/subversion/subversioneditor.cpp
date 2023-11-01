// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversioneditor.h"
#include "subversionplugin.h"

#include "annotationhighlighter.h"
#include "subversionconstants.h"
#include "subversiontr.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffandloghighlighter.h>

#include <QDebug>
#include <QFileInfo>
#include <QTextCursor>
#include <QTextBlock>

using namespace Subversion;
using namespace Subversion::Internal;

SubversionEditorWidget::SubversionEditorWidget() :
    m_changeNumberPattern("^\\s*(?<area>(?<rev>\\d+))\\s+.*$"),
    m_revisionNumberPattern("\\b(?<area>(r|[rR]evision )(?<rev>\\d+))\\b")
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
    setDiffFilePattern("^[-+]{3} ([^\\t]+)|^Index: .*|^=+$");
    setLogEntryPattern("^(r\\d+) \\|");
    setAnnotateRevisionTextFormat(Tr::tr("Annotate revision \"%1\""));
    setAnnotationEntryPattern("^(\\d+):");
}

QString SubversionEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::LineUnderCursor);
    if (!cursor.hasSelection())
        return {};
    const QString change = cursor.selectedText();
    const int pos = c.position() - cursor.selectionStart() + 1;
    // Annotation output has number, log output has revision numbers,
    // both at the start of the line.
    auto matchIter = m_changeNumberPattern.globalMatch(change);
    if (!matchIter.hasNext())
        matchIter = m_revisionNumberPattern.globalMatch(change);

    // We may have several matches of our regexp and we way have
    // several () in the regexp
    const QString areaName = QLatin1String("area");
    while (matchIter.hasNext()) {
        auto match = matchIter.next();
        const QString rev = match.captured(QLatin1String("rev"));
        if (rev.isEmpty())
            continue;

        const QString area = match.captured(areaName);
        QTC_ASSERT(area.contains(rev), continue);

        const int start = match.capturedStart(areaName);
        const int end = match.capturedEnd(areaName);
        if (pos > start && pos <= end)
            return rev;
    }
    return {};
}

VcsBase::BaseAnnotationHighlighter *SubversionEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new SubversionAnnotationHighlighter(changes);
}

QStringList SubversionEditorWidget::annotationPreviousVersions(const QString &v) const
{
    bool ok;
    const int revision = v.toInt(&ok);
    if (!ok || revision < 2)
        return {};
    return QStringList(QString::number(revision - 1));
}
