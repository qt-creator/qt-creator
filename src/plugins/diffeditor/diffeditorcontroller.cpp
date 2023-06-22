// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffeditorcontroller.h"

#include "diffeditorconstants.h"
#include "diffeditordocument.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <utils/qtcassert.h>

using namespace Core;
using namespace Tasking;
using namespace Utils;

namespace DiffEditor {

DiffEditorController::DiffEditorController(IDocument *document)
    : QObject(document)
    , m_document(qobject_cast<Internal::DiffEditorDocument *>(document))
    , m_reloadRecipe{}
{
    QTC_ASSERT(m_document, return);
    m_document->setController(this);
}

bool DiffEditorController::isReloading() const
{
    return m_taskTree.get() != nullptr;
}

FilePath DiffEditorController::workingDirectory() const
{
    return m_document->workingDirectory();
}

void DiffEditorController::setWorkingDirectory(const FilePath &directory)
{
    m_document->setWorkingDirectory(directory);
}

int DiffEditorController::contextLineCount() const
{
    return m_document->contextLineCount();
}

bool DiffEditorController::ignoreWhitespace() const
{
    return m_document->ignoreWhitespace();
}

QString DiffEditorController::makePatch(int fileIndex, int chunkIndex,
                                        const ChunkSelection &selection,
                                        PatchOptions options) const
{
    return m_document->makePatch(fileIndex, chunkIndex, selection,
                                 (options & Revert) ? PatchAction::Revert : PatchAction::Apply,
                                 options & AddPrefix);
}

IDocument *DiffEditorController::findOrCreateDocument(const QString &vcsId,
                                                      const QString &displayName)
{
    QString preferredDisplayName = displayName;
    IEditor *editor = EditorManager::openEditorWithContents(Constants::DIFF_EDITOR_ID,
                                                            &preferredDisplayName, {}, vcsId);
    return editor ? editor->document() : nullptr;
}

DiffEditorController *DiffEditorController::controller(IDocument *document)
{
    auto doc = qobject_cast<Internal::DiffEditorDocument *>(document);
    return doc ? doc->controller() : nullptr;
}

void DiffEditorController::setDiffFiles(const QList<FileData> &diffFileList)
{
    m_document->setDiffFiles(diffFileList);
}

void DiffEditorController::setDescription(const QString &description)
{
    m_document->setDescription(description);
}

/**
 * @brief Force the lines of context to the given number.
 *
 * The user will not be able to change the context lines anymore. This needs to be set before
 * starting any operation or the flag will be ignored by the UI.
 *
 * @param lines Lines of context to display.
 */
void DiffEditorController::forceContextLineCount(int lines)
{
    m_document->forceContextLineCount(lines);
}

IDocument *DiffEditorController::document() const
{
    return m_document;
}

/**
 * @brief Request the diff data to be re-read.
 */
void DiffEditorController::requestReload()
{
    m_document->beginReload();
    m_taskTree.reset(new TaskTree(m_reloadRecipe));
    connect(m_taskTree.get(), &TaskTree::done, this, [this] { reloadFinished(true); });
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, [this] { reloadFinished(false); });
    auto progress = new TaskProgress(m_taskTree.get());
    progress->setDisplayName(m_displayName);
    m_taskTree->start();
}

void DiffEditorController::reloadFinished(bool success)
{
    if (m_taskTree)
        m_taskTree.release()->deleteLater();
    m_document->endReload(success);
}

void DiffEditorController::addExtraActions(QMenu *menu, int fileIndex, int chunkIndex,
                                           const ChunkSelection &selection)
{
    Q_UNUSED(menu)
    Q_UNUSED(fileIndex)
    Q_UNUSED(chunkIndex)
    Q_UNUSED(selection)
}

void DiffEditorController::setStartupFile(const QString &startupFile)
{
    m_document->setStartupFile(startupFile);
}

bool DiffEditorController::chunkExists(int fileIndex, int chunkIndex) const
{
    if (!m_document)
        return false;

    if (fileIndex < 0 || chunkIndex < 0)
        return false;

    if (fileIndex >= m_document->diffFiles().count())
        return false;

    const FileData fileData = m_document->diffFiles().at(fileIndex);
    if (chunkIndex >= fileData.chunks.count())
        return false;

    return true;
}

} // namespace DiffEditor
