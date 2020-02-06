/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffeditordocument.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QStringList>

namespace DiffEditor {

DiffEditorController::DiffEditorController(Core::IDocument *document) :
    QObject(document),
    m_document(qobject_cast<Internal::DiffEditorDocument *>(document))
{
    QTC_ASSERT(m_document, return);
    m_document->setController(this);
}

bool DiffEditorController::isReloading() const
{
    return m_isReloading;
}

QString DiffEditorController::baseDirectory() const
{
    return m_document->baseDirectory();
}

void DiffEditorController::setBaseDirectory(const QString &directory)
{
    m_document->setBaseDirectory(directory);
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
                                 options & Revert, options & AddPrefix);
}

Core::IDocument *DiffEditorController::findOrCreateDocument(const QString &vcsId,
                                                            const QString &displayName)
{
    QString preferredDisplayName = displayName;
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                Constants::DIFF_EDITOR_ID, &preferredDisplayName, QByteArray(), vcsId);
    return editor ? editor->document() : nullptr;
}

DiffEditorController *DiffEditorController::controller(Core::IDocument *document)
{
    auto doc = qobject_cast<Internal::DiffEditorDocument *>(document);
    return doc ? doc->controller() : nullptr;
}

void DiffEditorController::setDiffFiles(const QList<FileData> &diffFileList,
                                        const QString &workingDirectory,
                                        const QString &startupFile)
{
    m_document->setDiffFiles(diffFileList, workingDirectory, startupFile);
}

void DiffEditorController::setDescription(const QString &description)
{
    m_document->setDescription(description);
}

QString DiffEditorController::description() const
{
    return m_document->description();
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

void DiffEditorController::setReloader(const std::function<void ()> &reloader)
{
    m_reloader = reloader;
}

Core::IDocument *DiffEditorController::document() const
{
    return m_document;
}

/**
 * @brief Request the diff data to be re-read.
 */
void DiffEditorController::requestReload()
{
    m_isReloading = true;
    m_document->beginReload();
    QTC_ASSERT(m_reloader, reloadFinished(false); return);
    m_reloader();
}

void DiffEditorController::reloadFinished(bool success)
{
    m_document->endReload(success);
    m_isReloading = false;
}

void DiffEditorController::requestChunkActions(QMenu *menu, int fileIndex, int chunkIndex,
                                               const ChunkSelection &selection)
{
    emit chunkActionsRequested(menu, fileIndex, chunkIndex, selection);
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
