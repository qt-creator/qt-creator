/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "bazaareditor.h"
#include "annotationhighlighter.h"
#include "constants.h"

#include <utils/qtcassert.h>

#include <QString>
#include <QTextCursor>

#define BZR_CHANGE_PATTERN "[0-9]+"

using namespace Bazaar::Internal;
using namespace Bazaar;

BazaarEditorWidget::BazaarEditorWidget() :
    m_changesetId(QLatin1String(Constants::CHANGESET_ID)),
    m_exactChangesetId(QLatin1String(Constants::CHANGESET_ID_EXACT))
{
    setAnnotateRevisionTextFormat(tr("&Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Annotate &parent revision %1"));
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
    return QString();
}

VcsBase::BaseAnnotationHighlighter *BazaarEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new BazaarAnnotationHighlighter(changes);
}
