// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditor_global.h"
#include "mergemodel.h"

#include <texteditor/textdocument.h>

#include <utils/result.h>

#include <QByteArray>

#include <array>
#include <functional>
#include <optional>

namespace Core { class IEditor; }

namespace DiffEditor {

class DIFFEDITOR_EXPORT MergeInput
{
public:
    using SnapshotCallback = std::function<void(const Utils::Result<MergeSnapshot> &)>;
    using MarkResolvedCallback = std::function<void(bool)>;
    using ConflictFreeCallback = std::function<void(bool)>;

    QString id;
    QString title;
    TextEditor::TextDocumentPtr resultDocument;
    std::optional<QByteArray> resultDiskContents;

    std::function<void(const SnapshotCallback &)> fetchBase;
    std::function<void(const SnapshotCallback &)> fetchOurs;
    std::function<void(const SnapshotCallback &)> fetchTheirs;

    // Optional Git/VCS check performed after the result has been saved.
    std::function<void(const ConflictFreeCallback &)> checkConflictFree;
    // Called after saving to mark the result as resolved. The callback reports
    // whether the operation completed successfully.
    std::function<void(const QByteArray &, const MarkResolvedCallback &)> markResolved;
    std::function<void()> refresh;
    std::function<bool(int, const QString &, const QString &)> openTextEditor;
};

DIFFEDITOR_EXPORT QObject *openMergeEditorFromFiles(
    const std::array<Utils::FilePath, 4> &files, QObject *parent = nullptr);

DIFFEDITOR_EXPORT Core::IEditor *openMergeEditor(
    const MergeInput &input, const QString &title = {});

#ifdef WITH_TESTS
namespace Internal { QObject *createMergeEditorTest(); }
#endif

} // namespace DiffEditor
