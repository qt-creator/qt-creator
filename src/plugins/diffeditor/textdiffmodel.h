// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditor_global.h"
#include "diffutils.h"

#include <utils/differ.h>

#include <QFuture>

#include <optional>

namespace DiffEditor {

// The result of computing one two-way text diff. The ChunkData keeps the
// existing aligned representation, while the newline flags are needed for
// changes that have no visible line of their own.
class DIFFEDITOR_EXPORT TextDiff
{
public:
    ChunkData chunk;
    bool leftEndsWithNewline = false;
    bool rightEndsWithNewline = false;
};

DIFFEDITOR_EXPORT TextDiff computeTextDiff(
    const QString &left,
    const QString &right,
    Utils::Differ::DiffAlgorithm algorithm = Utils::Differ::Default,
    const std::optional<QFuture<void>> &cancellation = {});

DIFFEDITOR_EXPORT QList<int> computeTextLineMapping(
    const QString &source, const QString &result,
    const std::optional<QFuture<void>> &cancellation = {});

} // namespace DiffEditor
