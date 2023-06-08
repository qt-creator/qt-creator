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

namespace Internal {
class DiffEditorDocument;
class DiffEditorWidgetController;
}

class ChunkSelection;

class DIFFEDITOR_EXPORT DiffEditorController : public QObject
{
    Q_OBJECT
public:
    explicit DiffEditorController(Core::IDocument *document);

    void requestReload();

    Utils::FilePath workingDirectory() const;
    void setWorkingDirectory(const Utils::FilePath &directory);

    enum PatchOption {
        NoOption = 0,
        Revert = 1,
        AddPrefix = 2
    };
    Q_DECLARE_FLAGS(PatchOptions, PatchOption)

    static Core::IDocument *findOrCreateDocument(const QString &vcsId, const QString &displayName);
    static DiffEditorController *controller(Core::IDocument *document);

protected:
    bool isReloading() const;
    int contextLineCount() const;
    bool ignoreWhitespace() const;
    bool chunkExists(int fileIndex, int chunkIndex) const;
    Core::IDocument *document() const;
    QString makePatch(int fileIndex, int chunkIndex, const ChunkSelection &selection,
                      PatchOptions options) const;

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
    friend class Internal::DiffEditorWidgetController;
    virtual void addExtraActions(QMenu *menu, int fileIndex, int chunkIndex,
                                 const ChunkSelection &selection);

    Internal::DiffEditorDocument *const m_document;
    QString m_displayName;
    std::unique_ptr<Tasking::TaskTree> m_taskTree;
    Tasking::Group m_reloadRecipe;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DiffEditorController::PatchOptions)

} // namespace DiffEditor
