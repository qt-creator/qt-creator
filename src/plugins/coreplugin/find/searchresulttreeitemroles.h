// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractItemView>
#include <QString>

namespace Core::Internal::ItemDataRoles {

enum Roles {
    ResultItemRole = Qt::UserRole,
    ResultLineRole,
    ResultBeginLineNumberRole,
    ResultIconRole,
    ResultHighlightBackgroundColor,
    ResultHighlightForegroundColor,
    FunctionHighlightBackgroundColor,
    FunctionHighlightForegroundColor,
    ResultBeginColumnNumberRole,
    ResultEndLineNumberRole,
    ResultEndColumnNumberRole,
    SearchTermLengthRole,
    ContainingFunctionNameRole,
    IsGroupingItemRole
};

} // namespace Core::Internal::ItemDataRoles

namespace Core::Internal {

// A multi-line result shows at most this many rows; if it has more lines, the last row is "...".
constexpr int maxDisplayedResultLines = 5;

// Lines in the given line text, counted up to one more than maxDisplayedResultLines.
inline int resultLineCount(const QString &lineText)
{
    int lines = 1;
    for (qsizetype pos = lineText.indexOf('\n'); pos >= 0 && lines <= maxDisplayedResultLines;
         pos = lineText.indexOf('\n', pos + 1)) {
        ++lines;
    }
    return lines;
}

// Rows a result with the given line text occupies in the view.
inline int displayedRowCount(const QString &lineText)
{
    return qMin(maxDisplayedResultLines, resultLineCount(lineText));
}

} // namespace Core::Internal
