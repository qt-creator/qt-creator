// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bazaareditor.h"

#include "annotationhighlighter.h"
#include "bazaartr.h"
#include "constants.h"

#include <utils/qtcassert.h>

#include <QString>
#include <QTextCursor>

#define BZR_CHANGE_PATTERN "[0-9]+"

namespace Bazaar::Internal {

BazaarEditorWidget::BazaarEditorWidget() :
    m_changesetId(QLatin1String(Constants::CHANGESET_ID)),
    m_exactChangesetId(QLatin1String(Constants::CHANGESET_ID_EXACT))
{
    setAnnotateRevisionTextFormat(Tr::tr("&Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(Tr::tr("Annotate &parent revision %1"));
    // Diff format:
    // === <change> <file|dir> 'mainwindow.cpp'
    setDiffFilePattern("^=== [a-z]+ [a-z]+ '(.+)'\\s*");
    setLogEntryPattern("^revno: (\\d+)");
    setAnnotationEntryPattern("^(" BZR_CHANGE_PATTERN ") ");
}

QString BazaarEditorWidget::changeUnderCursor(const QTextCursor &cursorIn) const
{
    // The test is done in two steps: first we check if the line contains a
    // changesetId. Then we check if the cursor is over the changesetId itself
    // and not over "revno" or another part of the line.
    // The two steps are necessary because matching only for the changesetId
    // leads to many false-positives (a regex like "[0-9]+" matches a lot of text).
    const int cursorCol = cursorIn.columnNumber();
    QTextCursor cursor = cursorIn;
    cursor.select(QTextCursor::LineUnderCursor);
    if (cursor.hasSelection()) {
        const QString line = cursor.selectedText();
        const QRegularExpressionMatch match = m_changesetId.match(line);
        if (match.hasMatch()) {
            const int start = match.capturedStart();
            const int stop = match.capturedEnd();
            if (start <= cursorCol && cursorCol <= stop) {
                cursor = cursorIn;
                cursor.select(QTextCursor::WordUnderCursor);
                if (cursor.hasSelection()) {
                    const QString change = cursor.selectedText();
                    if (m_exactChangesetId.match(change).hasMatch())
                        return change;
                }
            }
        }
    }
    return {};
}

VcsBase::BaseAnnotationHighlighter *BazaarEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new BazaarAnnotationHighlighter(changes);
}

} // Bazaar::Internal
