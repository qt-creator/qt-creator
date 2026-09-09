// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditor_global.h"

#include <QFuture>
#include <QList>
#include <QStringList>

#include <optional>

namespace DiffEditor {

// exists distinguishes a missing Git stage from an existing empty file.
class DIFFEDITOR_EXPORT MergeSnapshot
{
public:
    bool exists = true;
    QString text;
    QString displayName;
};

struct DIFFEDITOR_EXPORT MergeTextRange
{
    int startLine = 1; // 1-based
    int lineCount = 0;

    friend bool operator==(const MergeTextRange &, const MergeTextRange &) = default;
};

enum class MergeRegionKind {
    Unchanged,
    OursOnly,
    TheirsOnly,
    BothSame,
    AutoMerged,
    Conflict
};

struct DIFFEDITOR_EXPORT MergeRegion
{
    int id = -1;

    MergeTextRange base;
    MergeTextRange ours;
    MergeTextRange theirs;

    QStringList baseLines;
    QStringList oursLines;
    QStringList theirsLines;

    MergeRegionKind kind = MergeRegionKind::Unchanged;
};

class DIFFEDITOR_EXPORT MergeModel
{
public:
    bool computed = false;
    bool baseEndsWithNewline = false;
    bool oursEndsWithNewline = false;
    bool theirsEndsWithNewline = false;

    // Regions contain changes only. Unchanged ranges are implicit between
    // adjacent regions and can be recovered from the BASE snapshot.
    QList<MergeRegion> regions;
};

struct DIFFEDITOR_EXPORT MergeConflict
{
    QString originalText;
    QStringList ours;
    QStringList theirs;
    std::optional<QStringList> base;
};

struct DIFFEDITOR_EXPORT ParsedMergeResult
{
    QString text;
    QString placeholderPrefix;
    QList<MergeConflict> conflicts;
};

DIFFEDITOR_EXPORT ParsedMergeResult parseMergeResult(
    const QString &text, const QString &placeholderPrefix = "Unresolved conflict ");

DIFFEDITOR_EXPORT MergeModel computeMergeModel(
    const MergeSnapshot &base,
    const MergeSnapshot &ours,
    const MergeSnapshot &theirs,
    bool patience = false,
    const std::optional<QFuture<void>> &cancellation = {});

DIFFEDITOR_EXPORT MergeModel computeMergeModel(
    const QString &base,
    const QString &ours,
    const QString &theirs,
    bool patience = false);

} // namespace DiffEditor
