// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditor_global.h"
#include "diffutils.h"

#include <utils/filepath.h>
#include <utils/tasktree.h>

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Core { class IDocument; }

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

    Utils::FilePath baseDirectory() const;
    void setBaseDirectory(const Utils::FilePath &directory);
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
    void setDisplayName(const QString &name) { m_displayName = name; }
    QString displayName() const { return m_displayName; }

    // reloadFinished() should be called inside the reloader (for synchronous reload)
    // or later (for asynchronous reload)
    void setReloader(const std::function<void ()> &reloader);
    void reloadFinished(bool success);

    void setDiffFiles(const QList<FileData> &diffFileList,
                      const Utils::FilePath &baseDirectory = {},
                      const QString &startupFile = {});
    void setDescription(const QString &description);
    QString description() const;
    void forceContextLineCount(int lines);

private:
    Internal::DiffEditorDocument *const m_document;
    bool m_isReloading = false;
    QString m_displayName;
    std::function<void()> m_reloader;
    std::unique_ptr<Utils::TaskTree> m_taskTree;
    virtual Utils::Tasking::Group reloadRecipe() { return {}; } // TODO: make pure abstract

    friend class Internal::DiffEditorDocument;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DiffEditorController::PatchOptions)

} // namespace DiffEditor
