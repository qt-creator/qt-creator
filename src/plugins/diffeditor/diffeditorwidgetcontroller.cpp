// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffeditorwidgetcontroller.h"
#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffeditordocument.h"
#include "diffeditortr.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <cpaster/codepasterservice.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>

#include <utils/infobar.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QMenu>
#include <QTextCodec>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor::Internal {

DiffEditorWidgetController::DiffEditorWidgetController(QWidget *diffEditorWidget)
    : QObject(diffEditorWidget)
    , m_diffEditorWidget(diffEditorWidget)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(100);
    connect(&m_timer, &QTimer::timeout, this, &DiffEditorWidgetController::showProgress);
}

bool DiffEditorWidgetController::isInProgress() const
{
    return m_isBusyShowing || (m_document && m_document->state() == DiffEditorDocument::Reloading);
}

void DiffEditorWidgetController::toggleProgress(bool wasInProgress)
{
    const bool inProgress = isInProgress();
    if (wasInProgress == inProgress)
        return;

    if (inProgress)
        scheduleShowProgress();
    else
        hideProgress();
}

void DiffEditorWidgetController::setDocument(DiffEditorDocument *document)
{
    if (!m_progressIndicator) {
        m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large);
        m_progressIndicator->attachToWidget(m_diffEditorWidget);
        m_progressIndicator->hide();
    }

    if (m_document == document)
        return;

    if (m_document) {
        disconnect(m_document, &IDocument::aboutToReload, this, &DiffEditorWidgetController::scheduleShowProgress);
        disconnect(m_document, &IDocument::reloadFinished, this, &DiffEditorWidgetController::onDocumentReloadFinished);
    }

    const bool wasInProgress = isInProgress();

    m_document = document;
    if (m_document) {
        connect(m_document, &IDocument::aboutToReload, this, &DiffEditorWidgetController::scheduleShowProgress);
        connect(m_document, &IDocument::reloadFinished, this, &DiffEditorWidgetController::onDocumentReloadFinished);
        updateCannotDecodeInfo();
    }

    toggleProgress(wasInProgress);
}

DiffEditorDocument *DiffEditorWidgetController::document() const
{
    return m_document;
}

void DiffEditorWidgetController::setBusyShowing(bool busy)
{
    if (m_isBusyShowing == busy)
        return;

    const bool wasInProgress = isInProgress();
    m_isBusyShowing = busy;
    toggleProgress(wasInProgress);
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
    if (!isInProgress())
        hideProgress();
}

void DiffEditorWidgetController::patch(PatchAction patchAction, int fileIndex, int chunkIndex)
{
    if (!m_document)
        return;

    if (!chunkExists(fileIndex, chunkIndex))
        return;

    const FileData fileData = m_contextFileData.at(fileIndex);
    const QString fileName = patchAction == PatchAction::Apply
            ? fileData.fileInfo[LeftSide].fileName
            : fileData.fileInfo[RightSide].fileName;
    const DiffFileInfo::PatchBehaviour patchBehaviour = patchAction == PatchAction::Apply
            ? fileData.fileInfo[LeftSide].patchBehaviour
            : fileData.fileInfo[RightSide].patchBehaviour;

    const FilePath workingDirectory = m_document->workingDirectory().isEmpty()
            ? FilePath::fromString(fileName).absolutePath()
            : m_document->workingDirectory();
    const FilePath absFilePath = workingDirectory.resolvePath(fileName).absoluteFilePath();

    auto textDocument = qobject_cast<TextEditor::TextDocument *>(
        DocumentModel::documentForFilePath(absFilePath));
    const bool isModified = patchBehaviour == DiffFileInfo::PatchFile &&
            textDocument && textDocument->isModified();

    if (!PatchTool::confirmPatching(m_diffEditorWidget, patchAction, isModified))
        return;

    if (patchBehaviour == DiffFileInfo::PatchFile) {
        if (textDocument && !EditorManager::saveDocument(textDocument))
            return;
        const int strip = m_document->workingDirectory().isEmpty() ? -1 : 0;

        const QString patch = m_document->makePatch(fileIndex, chunkIndex, {}, patchAction);

        if (patch.isEmpty())
            return;

        FileChangeBlocker fileChangeBlocker(absFilePath);
        if (PatchTool::runPatch(EditorManager::defaultTextCodec()->fromUnicode(patch),
                                workingDirectory, strip, patchAction))
            m_document->reload();
    } else { // PatchEditor
        if (!textDocument)
            return;

        TemporaryFile contentsCopy("diff");
        if (!contentsCopy.open())
            return;

        contentsCopy.write(textDocument->contents());
        contentsCopy.close();

        const QString contentsCopyFileName = contentsCopy.fileName();
        const QString contentsCopyDir = QFileInfo(contentsCopyFileName).absolutePath();

        const QString patch = m_document->makePatch(fileIndex, chunkIndex, {}, patchAction, false,
                                                    QFileInfo(contentsCopyFileName).fileName());

        if (patch.isEmpty())
            return;

        if (PatchTool::runPatch(EditorManager::defaultTextCodec()->fromUnicode(patch),
                                FilePath::fromString(contentsCopyDir), 0, patchAction)) {
            QString errorString;
            if (textDocument->reload(&errorString, FilePath::fromString(contentsCopyFileName)))
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

    const FilePath filePath = m_document->workingDirectory().resolvePath(fileName);
    if (filePath.exists() && !filePath.isDir())
        EditorManager::openEditorAt({filePath, lineNumber, columnNumber});
}

void DiffEditorWidgetController::setFontSettings(const FontSettings &fontSettings)
{
    m_fileLineFormat        = fontSettings.toTextCharFormat(C_DIFF_FILE_LINE);
    m_chunkLineFormat       = fontSettings.toTextCharFormat(C_DIFF_CONTEXT_LINE);
    m_spanLineFormat        = fontSettings.toTextCharFormat(C_LINE_NUMBER);
    m_lineFormat[LeftSide]  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_LINE);
    m_charFormat[LeftSide]  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_CHAR);
    m_lineFormat[RightSide] = fontSettings.toTextCharFormat(C_DIFF_DEST_LINE);
    m_charFormat[RightSide] = fontSettings.toTextCharFormat(C_DIFF_DEST_CHAR);
}

void DiffEditorWidgetController::addCodePasterAction(QMenu *menu, int fileIndex, int chunkIndex)
{
    if (ExtensionSystem::PluginManager::getObject<CodePaster::Service>()) {
        // optional code pasting service
        QAction *sendChunkToCodePasterAction = menu->addAction(Tr::tr("Send Chunk to CodePaster..."));
        connect(sendChunkToCodePasterAction, &QAction::triggered, this, [this, fileIndex, chunkIndex] {
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
    if (!m_document || fileIndex < 0 || chunkIndex < 0 || fileIndex >= m_contextFileData.count())
        return {};

    const FileData fileData = m_contextFileData.at(fileIndex);
    if (chunkIndex >= fileData.chunks.count())
        return {};

    return fileData.chunks.at(chunkIndex);
}

bool DiffEditorWidgetController::fileNamesAreDifferent(int fileIndex) const
{
    const FileData fileData = m_contextFileData.at(fileIndex);
    return fileData.fileInfo[LeftSide].fileName != fileData.fileInfo[RightSide].fileName;
}

void DiffEditorWidgetController::addPatchAction(QMenu *menu, int fileIndex, int chunkIndex,
                                                PatchAction patchAction)
{
    const QString actionName = patchAction == PatchAction::Apply ? Tr::tr("Apply Chunk...")
                                                                 : Tr::tr("Revert Chunk...");
    QAction *action = menu->addAction(actionName);
    connect(action, &QAction::triggered, this, [this, fileIndex, chunkIndex, patchAction] {
        patch(patchAction, fileIndex, chunkIndex);
    });
    const bool enabled = chunkExists(fileIndex, chunkIndex)
            && (patchAction == PatchAction::Revert || fileNamesAreDifferent(fileIndex));
    action->setEnabled(enabled);
}

void DiffEditorWidgetController::addExtraActions(QMenu *menu, int fileIndex, int chunkIndex,
                                                 const ChunkSelection &selection)
{
    if (DiffEditorController *controller = m_document->controller())
        controller->addExtraActions(menu, fileIndex, chunkIndex, selection);
}

void DiffEditorWidgetController::updateCannotDecodeInfo()
{
    if (!m_document)
        return;

    InfoBar *infoBar = m_document->infoBar();
    Id selectEncodingId(Constants::SELECT_ENCODING);
    if (m_document->hasDecodingError()) {
        if (!infoBar->canInfoBeAdded(selectEncodingId))
            return;
        InfoBarEntry info(selectEncodingId,
                                 Tr::tr("<b>Error:</b> Could not decode \"%1\" with \"%2\"-encoding.")
                                     .arg(m_document->displayName(),
                                          QString::fromLatin1(m_document->codec()->name())));
        info.addCustomButton(Tr::tr("Select Encoding"), [this] { m_document->selectEncoding(); });
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

    const QString patch = m_document->makePatch(fileIndex, chunkIndex, {}, PatchAction::Apply);

    if (patch.isEmpty())
        return;

    pasteService->postText(patch, Constants::DIFF_EDITOR_MIMETYPE);
}

DiffEditorInput::DiffEditorInput(DiffEditorWidgetController *controller)
    : m_contextFileData(controller->m_contextFileData)
    , m_fileLineFormat(&controller->m_fileLineFormat)
    , m_chunkLineFormat(&controller->m_chunkLineFormat)
    , m_spanLineFormat(&controller->m_spanLineFormat)
    , m_lineFormat{&controller->m_lineFormat[LeftSide], &controller->m_lineFormat[RightSide]}
    , m_charFormat{&controller->m_charFormat[LeftSide], &controller->m_charFormat[RightSide]}
{ }

} // namespace DiffEditor::Internal
