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
#include "diffeditorcontroller.h"
#include "diffeditordocument.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/patchtool.h>

#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>

#include <extensionsystem/pluginmanager.h>

#include <cpaster/codepasterservice.h>

#include <utils/infobar.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QTextCodec>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

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
        m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Large);
        m_progressIndicator->attachToWidget(m_diffEditorWidget);
        m_progressIndicator->hide();
    }

    if (m_document == document)
        return;

    if (m_document) {
        disconnect(m_document, &IDocument::aboutToReload, this, &DiffEditorWidgetController::scheduleShowProgress);
        disconnect(m_document, &IDocument::reloadFinished, this, &DiffEditorWidgetController::onDocumentReloadFinished);
    }

    const bool wasRunning = m_document && m_document->state() == DiffEditorDocument::Reloading;

    m_document = document;

    if (m_document) {
        connect(m_document, &IDocument::aboutToReload, this, &DiffEditorWidgetController::scheduleShowProgress);
        connect(m_document, &IDocument::reloadFinished, this, &DiffEditorWidgetController::onDocumentReloadFinished);
        updateCannotDecodeInfo();
    }

    const bool isRunning = m_document && m_document->state() == DiffEditorDocument::Reloading;

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

void DiffEditorWidgetController::onDocumentReloadFinished()
{
    updateCannotDecodeInfo();
    hideProgress();
}

void DiffEditorWidgetController::patch(bool revert, int fileIndex, int chunkIndex)
{
    if (!m_document)
        return;

    if (!chunkExists(fileIndex, chunkIndex))
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

    const FileData fileData = m_contextFileData.at(fileIndex);
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

        const QString patch = m_document->makePatch(fileIndex, chunkIndex, ChunkSelection(), revert);

        if (patch.isEmpty())
            return;

        FileChangeBlocker fileChangeBlocker(absFileName);
        if (PatchTool::runPatch(EditorManager::defaultTextCodec()->fromUnicode(patch),
                                workingDirectory, strip, revert))
            m_document->reload();
    } else { // PatchEditor
        auto textDocument = qobject_cast<TextEditor::TextDocument *>(
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

        const QString patch = m_document->makePatch(fileIndex, chunkIndex,
                                                    ChunkSelection(), revert, false,
                                                    QFileInfo(contentsCopyFileName).fileName());

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

void DiffEditorWidgetController::addCodePasterAction(QMenu *menu, int fileIndex, int chunkIndex)
{
    if (ExtensionSystem::PluginManager::getObject<CodePaster::Service>()) {
        // optional code pasting service
        QAction *sendChunkToCodePasterAction = menu->addAction(tr("Send Chunk to CodePaster..."));
        connect(sendChunkToCodePasterAction, &QAction::triggered, this, [this, fileIndex, chunkIndex]() {
            sendChunkToCodePaster(fileIndex, chunkIndex);
        });
    }
}

bool DiffEditorWidgetController::chunkExists(int fileIndex, int chunkIndex) const
{
    if (!m_document)
        return false;

    if (DiffEditorController *controller = m_document->controller())
        return controller->chunkExists(fileIndex, chunkIndex);

    return false;
}

ChunkData DiffEditorWidgetController::chunkData(int fileIndex, int chunkIndex) const
{
    if (!m_document)
        return ChunkData();

    if (fileIndex < 0 || chunkIndex < 0)
        return ChunkData();

    if (fileIndex >= m_contextFileData.count())
        return ChunkData();

    const FileData fileData = m_contextFileData.at(fileIndex);
    if (chunkIndex >= fileData.chunks.count())
        return ChunkData();

    return fileData.chunks.at(chunkIndex);
}

bool DiffEditorWidgetController::fileNamesAreDifferent(int fileIndex) const
{
    const FileData fileData = m_contextFileData.at(fileIndex);
    return fileData.leftFileInfo.fileName != fileData.rightFileInfo.fileName;
}

void DiffEditorWidgetController::addApplyAction(QMenu *menu, int fileIndex, int chunkIndex)
{
    QAction *applyAction = menu->addAction(tr("Apply Chunk..."));
    connect(applyAction, &QAction::triggered, this, [this, fileIndex, chunkIndex]() {
        patch(false, fileIndex, chunkIndex);
    });
    applyAction->setEnabled(chunkExists(fileIndex, chunkIndex) && fileNamesAreDifferent(fileIndex));
}

void DiffEditorWidgetController::addRevertAction(QMenu *menu, int fileIndex, int chunkIndex)
{
    QAction *revertAction = menu->addAction(tr("Revert Chunk..."));
    connect(revertAction, &QAction::triggered, this, [this, fileIndex, chunkIndex]() {
        patch(true, fileIndex, chunkIndex);
    });
    revertAction->setEnabled(chunkExists(fileIndex, chunkIndex));
}

void DiffEditorWidgetController::addExtraActions(QMenu *menu, int fileIndex, int chunkIndex,
                                                 const ChunkSelection &selection)
{
    if (DiffEditorController *controller = m_document->controller())
        controller->requestChunkActions(menu, fileIndex, chunkIndex, selection);
}

void DiffEditorWidgetController::updateCannotDecodeInfo()
{
    if (!m_document)
        return;

    Utils::InfoBar *infoBar = m_document->infoBar();
    Id selectEncodingId(Constants::SELECT_ENCODING);
    if (m_document->hasDecodingError()) {
        if (!infoBar->canInfoBeAdded(selectEncodingId))
            return;
        Utils::InfoBarEntry info(selectEncodingId,
                                 tr("<b>Error:</b> Could not decode \"%1\" with \"%2\"-encoding.")
                                     .arg(m_document->displayName(),
                                          QString::fromLatin1(m_document->codec()->name())));
        info.setCustomButtonInfo(tr("Select Encoding"), [this]() { m_document->selectEncoding(); });
        infoBar->addInfo(info);
    } else {
        infoBar->removeInfo(selectEncodingId);
    }
}

void DiffEditorWidgetController::sendChunkToCodePaster(int fileIndex, int chunkIndex)
{
    if (!m_document)
        return;

    // Retrieve service by soft dependency.
    auto pasteService = ExtensionSystem::PluginManager::getObject<CodePaster::Service>();
    QTC_ASSERT(pasteService, return);

    const QString patch = m_document->makePatch(fileIndex, chunkIndex,
                                                ChunkSelection(), false);

    if (patch.isEmpty())
        return;

    pasteService->postText(patch, Constants::DIFF_EDITOR_MIMETYPE);
}

} // namespace Internal
} // namespace DiffEditor
