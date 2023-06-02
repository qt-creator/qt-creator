// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditor_global.h"
#include "diffutils.h"

#include <solutions/tasking/tasktree.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Core { class IDocument; }
namespace Utils { class FilePath; }

namespace DiffEditor {

namespace Internal { class DiffEditorDocument; }

class ChunkSelection;

class DIFFEDITOR_EXPORT DiffEditorController : public QObject
{
    Q_OBJECT
public:
    explicit DiffEditorController(Core::IDocument *document);

    void requestReload();
    bool isReloading() const;

    Utils::FilePath workingDirectory() const;
    void setWorkingDirectory(const Utils::FilePath &directory);
    int contextLineCount() const;
    bool ignoreWhitespace() const;

    enum PatchOption {
        NoOption = 0,
        Revert = 1,
        AddPrefix = 2
    };
    Q_DECLARE_FLAGS(PatchOptions, PatchOption)
    QString makePatch(int fileIndex, int chunkIndex, const ChunkSelection &selection,
                      PatchOptions options) const;

    static Core::IDocument *findOrCreateDocument(const QString &vcsId,
                                                 const QString &displayName);
    static DiffEditorController *controller(Core::IDocument *document);

    void requestChunkActions(QMenu *menu, int fileIndex, int chunkIndex,
                             const ChunkSelection &selection);
    bool chunkExists(int fileIndex, int chunkIndex) const;
    Core::IDocument *document() const;

signals:
    void chunkActionsRequested(QMenu *menu, int fileIndex, int chunkIndex,
                               const ChunkSelection &selection);

protected:
    // Core functions:
    void setReloadRecipe(const Tasking::Group &recipe) { m_reloadRecipe = recipe; }
    void setDiffFiles(const QList<FileData> &diffFileList);
    // Optional:
    void setDisplayName(const QString &name) { m_displayName = name; }
    void setDescription(const QString &description);
    void setStartupFile(const QString &startupFile);
    void forceContextLineCount(int lines);

private:
    void reloadFinished(bool success);

    Internal::DiffEditorDocument *const m_document;
    QString m_displayName;
    std::unique_ptr<Tasking::TaskTree> m_taskTree;
    Tasking::Group m_reloadRecipe;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DiffEditorController::PatchOptions)

} // namespace DiffEditor
