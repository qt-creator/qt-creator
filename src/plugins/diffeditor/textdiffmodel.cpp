// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textdiffmodel.h"

#include <utils/differ.h>

using namespace Utils;

namespace DiffEditor {

TextDiff computeTextDiff(const QString &left, const QString &right, Utils::Differ::DiffAlgorithm algorithm,
                         const std::optional<QFuture<void>> &cancellation)
{
    TextDiff result;
    result.leftEndsWithNewline = left.endsWith('\n');
    result.rightEndsWithNewline = right.endsWith('\n');

    if (cancellation && cancellation->isCanceled())
        return result;
    if (left == right)
        return result;

    Differ differ(cancellation);
    differ.setDiffAlgorithm(algorithm);
    const QList<Diff> diffList = Differ::cleanupLineDiffSemantics(differ.diff(left, right));
    if (cancellation && cancellation->isCanceled())
        return result;

    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
    if (cancellation && cancellation->isCanceled())
        return result;
    result.chunk = DiffUtils::calculateOriginalData(leftDiffList, rightDiffList);
    return result;
}

QList<int> computeTextLineMapping(const QString &source, const QString &result,
                                  const std::optional<QFuture<void>> &cancellation)
{
    if (cancellation && cancellation->isCanceled())
        return {};
    const TextDiff diff = computeTextDiff(source, result, Differ::Default, cancellation);
    if (cancellation && cancellation->isCanceled())
        return {};
    QList<int> mapping;
    const int sourceLines = source.count('\n') + 1;
    if (source == result) {
        for (int line = 1; line <= sourceLines; ++line)
            mapping.append(line);
        return mapping;
    }
    int sourceLine = 0;
    for (const RowData &row : diff.chunk.rows) {
        if (cancellation && cancellation->isCanceled())
            return {};
        if (row.line[LeftSide].textLineType == TextLineData::TextLine)
            ++sourceLine;
        if (row.line[RightSide].textLineType == TextLineData::TextLine)
            mapping.append(qBound(1, sourceLine, sourceLines));
    }
    const int resultLines = result.count('\n') + 1;
    while (mapping.size() < resultLines) {
        if (cancellation && cancellation->isCanceled())
            return {};
        mapping.append(sourceLines);
    }
    return mapping;
}

} // namespace DiffEditor
