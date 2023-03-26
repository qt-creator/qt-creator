// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clearcaseeditor.h"
#include "clearcasetr.h"

#include "annotationhighlighter.h"

#include <utils/qtcassert.h>

#include <QTextCursor>

namespace ClearCase::Internal {

ClearCaseEditorWidget::ClearCaseEditorWidget() :
    m_versionNumberPattern(QLatin1String("[\\\\/]main[\\\\/][^ \t\n\"]*"))
{
    QTC_ASSERT(m_versionNumberPattern.isValid(), return);
    // Diff formats:
    // "+++ D:\depot\...\mainwindow.cpp@@\main\3" (versioned)
    // "+++ D:\depot\...\mainwindow.cpp[TAB]Sun May 01 14:22:37 2011" (local)
    setDiffFilePattern("^[-+]{3} ([^\\t]+?)(?:@@|\\t)");
    setLogEntryPattern("version \"([^\"]+)\"");
    setAnnotateRevisionTextFormat(Tr::tr("Annotate version \"%1\""));
    setAnnotationEntryPattern("([^|]*)\\|[^\\n]*\\n");
    setAnnotationSeparatorPattern("\\n-{30}");
}

QString ClearCaseEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::BlockUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    const QString change = cursor.selectedText();
    // Annotation output has number, log output has revision numbers
    // as r1, r2...
    const QRegularExpressionMatch match = m_versionNumberPattern.match(change);
    if (match.hasMatch())
        return match.captured();
    return QString();
}

VcsBase::BaseAnnotationHighlighter *ClearCaseEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new ClearCaseAnnotationHighlighter(changes);
}

} // ClearCase::Internal
