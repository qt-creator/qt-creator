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

#include "diffeditorwidgetcontroller.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/patchtool.h>

#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>

#include <extensionsystem/pluginmanager.h>

#include <cpaster/codepasterservice.h>

#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QTextCodec>

using namespace Core;
using namespace TextEditor;

namespace DiffEditor {
namespace Internal {

DiffEditorWidgetController::DiffEditorWidgetController(QWidget *diffEditorWidget)
    : QObject(diffEditorWidget)
    , m_diffEditorWidget(diffEditorWidget)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(100);
    connect(&m_timer, &QTimer::timeout, this, &DiffEditorWidgetController::showProgress);
}

void DiffEditorWidgetController::setDocument(DiffEditorDocument *document)
{
    if (!m_progressIndicator) {
        m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicator::Large);
        m_progressIndicator->attachToWidget(m_diffEditorWidget);
        m_progressIndicator->hide();
    }

    if (m_document == document)
        return;

    if (m_document) {
        disconnect(m_document, &IDocument::aboutToReload, this, &DiffEditorWidgetController::scheduleShowProgress);
        disconnect(m_document, &IDocument::reloadFinished, this, &DiffEditorWidgetController::hideProgress);
    }

    const bool wasRunning = m_document && m_document->isReloading();

    m_document = document;

    if (m_document) {
        connect(m_document, &IDocument::aboutToReload, this, &DiffEditorWidgetController::scheduleShowProgress);
        connect(m_document, &IDocument::reloadFinished, this, &DiffEditorWidgetController::hideProgress);
    }

    const bool isRunning = m_document && m_document->isReloading();

    if (wasRunning == isRunning)
        return;

    if (isRunning)
        scheduleShowProgress();
    else
        hideProgress();
}

DiffEditorDocument *DiffEditorWidgetController::document() const
{
    return m_document;
}

void DiffEditorWidgetController::scheduleShowProgress()
{
    m_timer.start();
}

void DiffEditorWidgetController::showProgress()
{
    m_timer.stop();
    if (m_progressIndicator)
        m_progressIndicator->show();
}

void DiffEditorWidgetController::hideProgress()
{
    m_timer.stop();
    if (m_progressIndicator)
        m_progressIndicator->hide();
}

void DiffEditorWidgetController::patch(bool revert)
{
    if (!m_document)
        return;

    const QString title = revert ? tr("Revert Chunk") : tr("Apply Chunk");
    const QString question = revert
            ? tr("Would you like to revert the chunk?")
            : tr("Would you like to apply the chunk?");
    if (QMessageBox::No == QMessageBox::question(m_diffEditorWidget, title,
                                                 question,
                                                 QMessageBox::Yes
                                                 | QMessageBox::No)) {
        return;
    }

    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);
    const QString fileName = revert
            ? fileData.rightFileInfo.fileName
            : fileData.leftFileInfo.fileName;
    const DiffFileInfo::PatchBehaviour patchBehaviour = revert
            ? fileData.rightFileInfo.patchBehaviour
            : fileData.leftFileInfo.patchBehaviour;

    const QString workingDirectory = m_document->baseDirectory().isEmpty()
            ? QFileInfo(fileName).absolutePath()
            : m_document->baseDirectory();
    const QString absFileName = QFileInfo(workingDirectory + '/' + QFileInfo(fileName).fileName()).absoluteFilePath();

    if (patchBehaviour == DiffFileInfo::PatchFile) {
        const int strip = m_document->baseDirectory().isEmpty() ? -1 : 0;

        const QString patch = m_document->makePatch(m_contextMenuFileIndex, m_contextMenuChunkIndex, revert);

        if (patch.isEmpty())
            return;

        FileChangeBlocker fileChangeBlocker(absFileName);
        if (PatchTool::runPatch(EditorManager::defaultTextCodec()->fromUnicode(patch),
                                workingDirectory, strip, revert))
            m_document->reload();
    } else { // PatchEditor
        TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(
                    DocumentModel::documentForFilePath(absFileName));
        if (!textDocument)
            return;

        Utils::TemporaryFile contentsCopy("diff");
        if (!contentsCopy.open())
            return;

        contentsCopy.write(textDocument->contents());
        contentsCopy.close();

        const QString contentsCopyFileName = contentsCopy.fileName();
        const QString contentsCopyDir = QFileInfo(contentsCopyFileName).absolutePath();

        const QString patch = m_document->makePatch(m_contextMenuFileIndex,
                              m_contextMenuChunkIndex, revert, false, QFileInfo(contentsCopyFileName).fileName());

        if (patch.isEmpty())
            return;

        if (PatchTool::runPatch(EditorManager::defaultTextCodec()->fromUnicode(patch),
                                contentsCopyDir, 0, revert)) {
            QString errorString;
            if (textDocument->reload(&errorString, contentsCopyFileName))
                m_document->reload();
        }
    }
}

void DiffEditorWidgetController::jumpToOriginalFile(const QString &fileName,
                                                    int lineNumber,
                                                    int columnNumber)
{
    if (!m_document)
        return;

    const QDir dir(m_document->baseDirectory());
    const QString absoluteFileName = dir.absoluteFilePath(fileName);
    const QFileInfo fi(absoluteFileName);
    if (fi.exists() && !fi.isDir())
        EditorManager::openEditorAt(absoluteFileName, lineNumber, columnNumber);
}

void DiffEditorWidgetController::setFontSettings(const FontSettings &fontSettings)
{
    m_fileLineFormat  = fontSettings.toTextCharFormat(C_DIFF_FILE_LINE);
    m_chunkLineFormat = fontSettings.toTextCharFormat(C_DIFF_CONTEXT_LINE);
    m_leftLineFormat  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_LINE);
    m_leftCharFormat  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_CHAR);
    m_rightLineFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_LINE);
    m_rightCharFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_CHAR);
}

void DiffEditorWidgetController::addCodePasterAction(QMenu *menu)
{
    if (ExtensionSystem::PluginManager::getObject<CodePaster::Service>()) {
        // optional code pasting service
        QAction *sendChunkToCodePasterAction = menu->addAction(tr("Send Chunk to CodePaster..."));
        connect(sendChunkToCodePasterAction, &QAction::triggered,
                this, &DiffEditorWidgetController::slotSendChunkToCodePaster);
    }
}

bool DiffEditorWidgetController::setAndVerifyIndexes(QMenu *menu,
                                                     int diffFileIndex, int chunkIndex)
{
    if (!m_document)
        return false;

    m_contextMenuFileIndex = diffFileIndex;
    m_contextMenuChunkIndex = chunkIndex;

    if (m_contextMenuFileIndex < 0 || m_contextMenuChunkIndex < 0)
        return false;

    if (m_contextMenuFileIndex >= m_contextFileData.count())
        return false;

    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);
    if (m_contextMenuChunkIndex >= fileData.chunks.count())
        return false;

    m_document->chunkActionsRequested(menu, diffFileIndex, chunkIndex);

    return true;
}

bool DiffEditorWidgetController::fileNamesAreDifferent() const
{
    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);
    return fileData.leftFileInfo.fileName != fileData.rightFileInfo.fileName;
}

void DiffEditorWidgetController::addApplyAction(QMenu *menu, int diffFileIndex,
                                                int chunkIndex)
{
    QAction *applyAction = menu->addAction(tr("Apply Chunk..."));
    connect(applyAction, &QAction::triggered, this, &DiffEditorWidgetController::slotApplyChunk);
    applyAction->setEnabled(setAndVerifyIndexes(menu, diffFileIndex, chunkIndex)
                            && fileNamesAreDifferent());
}

void DiffEditorWidgetController::addRevertAction(QMenu *menu, int diffFileIndex,
                                                 int chunkIndex)
{
    QAction *revertAction = menu->addAction(tr("Revert Chunk..."));
    connect(revertAction, &QAction::triggered, this, &DiffEditorWidgetController::slotRevertChunk);
    revertAction->setEnabled(setAndVerifyIndexes(menu, diffFileIndex, chunkIndex));
}

void DiffEditorWidgetController::slotSendChunkToCodePaster()
{
    if (!m_document)
        return;

    // Retrieve service by soft dependency.
    auto pasteService = ExtensionSystem::PluginManager::getObject<CodePaster::Service>();
    QTC_ASSERT(pasteService, return);

    const QString patch = m_document->makePatch(m_contextMenuFileIndex, m_contextMenuChunkIndex, false);

    if (patch.isEmpty())
        return;

    pasteService->postText(patch, QLatin1String(Constants::DIFF_EDITOR_MIMETYPE));
}

void DiffEditorWidgetController::slotApplyChunk()
{
    patch(false);
}

void DiffEditorWidgetController::slotRevertChunk()
{
    patch(true);
}

} // namespace Internal
} // namespace DiffEditor
