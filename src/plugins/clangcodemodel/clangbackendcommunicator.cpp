/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangbackendcommunicator.h"

#include "clangbackendlogging.h"
#include "clangcompletionassistprocessor.h"
#include "clangmodelmanagersupport.h"
#include "clangutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <cpptools/abstracteditorsupport.h>
#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/editordocumenthandle.h>
#include <cpptools/projectinfo.h>
#include <cpptools/cpptoolsbridge.h>

#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/texteditor.h>

#include <clangsupport/filecontainer.h>
#include <clangsupport/clangcodemodelservermessages.h>

#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDir>
#include <QTextBlock>

using namespace CPlusPlus;
using namespace ClangBackEnd;
using namespace TextEditor;

enum { backEndStartTimeOutInMs = 10000 };

static QString backendProcessPath()
{
    return Core::ICore::libexecPath()
            + QStringLiteral("/clangbackend")
            + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

namespace ClangCodeModel {
namespace Internal {

class DummyBackendSender : public ClangBackEnd::ClangCodeModelServerInterface
{
public:
    void end() override {}

    void documentsOpened(const DocumentsOpenedMessage &) override {}
    void documentsChanged(const DocumentsChangedMessage &) override {}
    void documentsClosed(const DocumentsClosedMessage &) override {}
    void documentVisibilityChanged(const DocumentVisibilityChangedMessage &) override {}

    void projectPartsUpdated(const ProjectPartsUpdatedMessage &) override {}
    void projectPartsRemoved(const ProjectPartsRemovedMessage &) override {}

    void unsavedFilesUpdated(const UnsavedFilesUpdatedMessage &) override {}
    void unsavedFilesRemoved(const UnsavedFilesRemovedMessage &) override {}

    void requestCompletions(const RequestCompletionsMessage &) override {}
    void requestAnnotations(const RequestAnnotationsMessage &) override {}
    void requestReferences(const RequestReferencesMessage &) override {}
    void requestFollowSymbol(const RequestFollowSymbolMessage &) override {}
    void requestToolTip(const RequestToolTipMessage &) override {}
};

BackendCommunicator::BackendCommunicator()
    : m_connection(&m_receiver)
    , m_sender(new DummyBackendSender())
{
    m_backendStartTimeOut.setSingleShot(true);
    connect(&m_backendStartTimeOut, &QTimer::timeout,
            this, &BackendCommunicator::logStartTimeOut);

    m_receiver.setAliveHandler([this]() { m_connection.resetProcessAliveTimer(); });

    connect(Core::EditorManager::instance(), &Core::EditorManager::editorAboutToClose,
            this, &BackendCommunicator::onEditorAboutToClose);
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
            this, &BackendCommunicator::setupDummySender);

    initializeBackend();
}

BackendCommunicator::~BackendCommunicator()
{
    disconnect(&m_connection, 0, this, 0);
}

void BackendCommunicator::initializeBackend()
{
    const QString clangBackEndProcessPath = backendProcessPath();
    if (!QFileInfo(clangBackEndProcessPath).exists()) {
        logExecutableDoesNotExist();
        return;
    }
    qCDebug(ipcLog) << "Starting" << clangBackEndProcessPath;

    m_connection.setProcessAliveTimerInterval(30 * 1000);
    m_connection.setProcessPath(clangBackEndProcessPath);

    connect(&m_connection, &ConnectionClient::connectedToLocalSocket,
            this, &BackendCommunicator::onConnectedToBackend);
    connect(&m_connection, &ConnectionClient::disconnectedFromLocalSocket,
            this, &BackendCommunicator::setupDummySender);

    m_connection.startProcessAndConnectToServerAsynchronously();
    m_backendStartTimeOut.start(backEndStartTimeOutInMs);
}

static QStringList projectPartOptions(const CppTools::ProjectPart::Ptr &projectPart)
{
    const QStringList options = ClangCodeModel::Utils::createClangOptions(projectPart,
        CppTools::ProjectFile::Unsupported); // No language option

    return options;
}

static ProjectPartContainer toProjectPartContainer(
        const CppTools::ProjectPart::Ptr &projectPart)
{
    const QStringList options = projectPartOptions(projectPart);

    return ProjectPartContainer(projectPart->id(), Utf8StringVector(options));
}

static QVector<ProjectPartContainer> toProjectPartContainers(
        const QVector<CppTools::ProjectPart::Ptr> projectParts)
{
    QVector<ProjectPartContainer> projectPartContainers;
    projectPartContainers.reserve(projectParts.size());

    foreach (const CppTools::ProjectPart::Ptr &projectPart, projectParts)
        projectPartContainers << toProjectPartContainer(projectPart);

    return projectPartContainers;
}

void BackendCommunicator::projectPartsUpdatedForFallback()
{
    const auto projectPart = CppTools::CppModelManager::instance()->fallbackProjectPart();
    const auto projectPartContainer = toProjectPartContainer(projectPart);

    projectPartsUpdated({projectPartContainer});
}

namespace {
Utf8String currentCppEditorDocumentFilePath()
{
    Utf8String currentCppEditorDocumentFilePath;

    const auto currentEditor = Core::EditorManager::currentEditor();
    if (currentEditor && CppTools::CppModelManager::isCppEditor(currentEditor)) {
        const auto currentDocument = currentEditor->document();
        if (currentDocument)
            currentCppEditorDocumentFilePath = currentDocument->filePath().toString();
    }

    return currentCppEditorDocumentFilePath;
}

void removeDuplicates(Utf8StringVector &visibleEditorDocumentsFilePaths)
{
    std::sort(visibleEditorDocumentsFilePaths.begin(),
              visibleEditorDocumentsFilePaths.end());
    const auto end = std::unique(visibleEditorDocumentsFilePaths.begin(),
                                 visibleEditorDocumentsFilePaths.end());
    visibleEditorDocumentsFilePaths.erase(end,
                                          visibleEditorDocumentsFilePaths.end());
}

void removeNonCppEditors(QList<Core::IEditor*> &visibleEditors)
{
    const auto isNotCppEditor = [] (Core::IEditor *editor) {
        return !CppTools::CppModelManager::isCppEditor(editor);
    };

    const auto end = std::remove_if(visibleEditors.begin(),
                                    visibleEditors.end(),
                                    isNotCppEditor);

    visibleEditors.erase(end, visibleEditors.end());
}

Utf8StringVector visibleCppEditorDocumentsFilePaths()
{
    auto visibleEditors = CppTools::CppToolsBridge::visibleEditors();

    removeNonCppEditors(visibleEditors);

    Utf8StringVector visibleCppEditorDocumentsFilePaths;
    visibleCppEditorDocumentsFilePaths.reserve(visibleEditors.size());

    const auto editorFilePaths = [] (Core::IEditor *editor) {
        return Utf8String(editor->document()->filePath().toString());
    };

    std::transform(visibleEditors.begin(),
                   visibleEditors.end(),
                   std::back_inserter(visibleCppEditorDocumentsFilePaths),
                   editorFilePaths);

    removeDuplicates(visibleCppEditorDocumentsFilePaths);

    return visibleCppEditorDocumentsFilePaths;
}

}

void BackendCommunicator::documentVisibilityChanged()
{
    documentVisibilityChanged(currentCppEditorDocumentFilePath(), visibleCppEditorDocumentsFilePaths());
}

bool BackendCommunicator::isNotWaitingForCompletion() const
{
    return !m_receiver.isExpectingCompletionsMessage();
}

void BackendCommunicator::documentVisibilityChanged(const Utf8String &currentEditorFilePath,
                                                    const Utf8StringVector &visibleEditorsFilePaths)
{
    const DocumentVisibilityChangedMessage message(currentEditorFilePath, visibleEditorsFilePaths);
    m_sender->documentVisibilityChanged(message);
}

void BackendCommunicator::projectPartsUpdatedForCurrentProjects()
{
    using namespace CppTools;

    const QList<ProjectInfo> projectInfos = CppModelManager::instance()->projectInfos();
    foreach (const ProjectInfo &projectInfo, projectInfos)
        projectPartsUpdated(projectInfo.projectParts());
}

void BackendCommunicator::restoreCppEditorDocuments()
{
    resetCppEditorDocumentProcessors();
    CppTools::CppModelManager::instance()->updateCppEditorDocuments();
}

void BackendCommunicator::resetCppEditorDocumentProcessors()
{
    using namespace CppTools;

    const auto cppEditorDocuments = CppModelManager::instance()->cppEditorDocuments();
    foreach (CppEditorDocumentHandle *cppEditorDocument, cppEditorDocuments)
        cppEditorDocument->resetProcessor();
}

void BackendCommunicator::unsavedFilesUpdatedForUiHeaders()
{
    using namespace CppTools;

    const auto editorSupports = CppModelManager::instance()->abstractEditorSupports();
    foreach (const AbstractEditorSupport *es, editorSupports) {
        const QString mappedPath
                = ModelManagerSupportClang::instance()->dummyUiHeaderOnDiskPath(es->fileName());
        unsavedFilesUpdated(mappedPath, es->contents(), es->revision());
    }
}

void BackendCommunicator::projectPartsUpdated(const QVector<CppTools::ProjectPart::Ptr> projectParts)
{
    const auto projectPartContainers = toProjectPartContainers(projectParts);
    projectPartsUpdated(projectPartContainers);
}

void BackendCommunicator::documentsChangedFromCppEditorDocument(const QString &filePath)
{
    const CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath);

    documentsChanged(filePath, document->contents(), document->revision());
}

void BackendCommunicator::unsavedFielsUpdatedFromCppEditorDocument(const QString &filePath)
{
    const CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath);

    unsavedFilesUpdated(filePath, document->contents(), document->revision());
}

void BackendCommunicator::documentsChanged(const QString &filePath,
                                           const QByteArray &contents,
                                           uint documentRevision)
{
    const bool hasUnsavedContent = true;

    documentsChanged({{filePath,
                       Utf8String(),
                       Utf8String::fromByteArray(contents),
                       hasUnsavedContent,
                       documentRevision}});
}

void BackendCommunicator::unsavedFilesUpdated(const QString &filePath,
                                              const QByteArray &contents,
                                              uint documentRevision)
{
    const bool hasUnsavedContent = true;

    // TODO: Send new only if changed
    unsavedFilesUpdated({{filePath,
                          Utf8String(),
                          Utf8String::fromByteArray(contents),
                          hasUnsavedContent,
                          documentRevision}});
}

static bool documentHasChanged(const QString &filePath, uint revision)
{
    if (CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath))
        return document->sendTracker().shouldSendRevision(revision);

    return true;
}

static void setLastSentDocumentRevision(const QString &filePath, uint revision)
{
    if (CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath))
        document->sendTracker().setLastSentRevision(int(revision));
}

void BackendCommunicator::documentsChangedWithRevisionCheck(const FileContainer &fileContainer)
{
    if (documentHasChanged(fileContainer.filePath, fileContainer.documentRevision)) {
        documentsChanged({fileContainer});
        setLastSentDocumentRevision(fileContainer.filePath,
                                    fileContainer.documentRevision);
    }
}

void BackendCommunicator::requestAnnotations(const FileContainer &fileContainer)
{
    const RequestAnnotationsMessage message(fileContainer);
    m_sender->requestAnnotations(message);
}

QFuture<CppTools::CursorInfo> BackendCommunicator::requestReferences(
        const FileContainer &fileContainer,
        quint32 line,
        quint32 column,
        const CppTools::SemanticInfo::LocalUseMap &localUses)
{
    const RequestReferencesMessage message(fileContainer, line, column);
    m_sender->requestReferences(message);

    return m_receiver.addExpectedReferencesMessage(message.ticketNumber, localUses);
}

QFuture<CppTools::CursorInfo> BackendCommunicator::requestLocalReferences(
        const FileContainer &fileContainer,
        quint32 line,
        quint32 column)
{
    const RequestReferencesMessage message(fileContainer, line, column, true);
    m_sender->requestReferences(message);

    return m_receiver.addExpectedReferencesMessage(message.ticketNumber);
}

QFuture<CppTools::ToolTipInfo> BackendCommunicator::requestToolTip(
        const FileContainer &fileContainer, quint32 line, quint32 column)
{
    const RequestToolTipMessage message(fileContainer, line, column);
    m_sender->requestToolTip(message);

    return m_receiver.addExpectedToolTipMessage(message.ticketNumber);
}

QFuture<CppTools::SymbolInfo> BackendCommunicator::requestFollowSymbol(
        const FileContainer &curFileContainer,
        quint32 line,
        quint32 column)
{
    const RequestFollowSymbolMessage message(curFileContainer, line, column);
    m_sender->requestFollowSymbol(message);

    return m_receiver.addExpectedRequestFollowSymbolMessage(message.ticketNumber);
}

void BackendCommunicator::documentsChangedWithRevisionCheck(Core::IDocument *document)
{
    const auto textDocument = qobject_cast<TextDocument*>(document);
    const auto filePath = textDocument->filePath().toString();
    const QString projectPartId = CppTools::CppToolsBridge::projectPartIdForFile(filePath);

    documentsChangedWithRevisionCheck(FileContainer(filePath,
                                                    projectPartId,
                                                    Utf8StringVector(),
                                                    textDocument->document()->revision()));
}

void BackendCommunicator::updateChangeContentStartPosition(const QString &filePath, int position)
{
    if (CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath))
        document->sendTracker().applyContentChange(position);
}

void BackendCommunicator::documentsChangedIfNotCurrentDocument(Core::IDocument *document)
{
    QTC_ASSERT(document, return);
    if (Core::EditorManager::currentDocument() != document)
        documentsChanged(document);
}

void BackendCommunicator::documentsChanged(Core::IDocument *document)
{
    documentsChangedFromCppEditorDocument(document->filePath().toString());
}

void BackendCommunicator::unsavedFilesUpdated(Core::IDocument *document)
{
    QTC_ASSERT(document, return);

     unsavedFielsUpdatedFromCppEditorDocument(document->filePath().toString());
}

void BackendCommunicator::onConnectedToBackend()
{
    m_backendStartTimeOut.stop();

    ++m_connectedCount;
    if (m_connectedCount > 1)
        logRestartedDueToUnexpectedFinish();

    m_receiver.reset();
    m_sender.reset(new BackendSender(&m_connection));

    initializeBackendWithCurrentData();
}

void BackendCommunicator::onEditorAboutToClose(Core::IEditor *editor)
{
    if (auto *textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor))
        m_receiver.deleteProcessorsOfEditorWidget(textEditor->editorWidget());
}

void BackendCommunicator::setupDummySender()
{
    m_sender.reset(new DummyBackendSender);
}

void BackendCommunicator::logExecutableDoesNotExist()
{
    const QString msg
        = tr("Clang Code Model: Error: "
             "The clangbackend executable \"%1\" does not exist.")
                .arg(QDir::toNativeSeparators(backendProcessPath()));

    logError(msg);
}

void BackendCommunicator::logStartTimeOut()
{
    const QString msg
        = tr("Clang Code Model: Error: "
             "The clangbackend executable \"%1\" could not be started (timeout after %2ms).")
                .arg(QDir::toNativeSeparators(backendProcessPath()))
                .arg(backEndStartTimeOutInMs);

    logError(msg);
}

void BackendCommunicator::logRestartedDueToUnexpectedFinish()
{
    const QString msg
        = tr("Clang Code Model: Error: "
             "The clangbackend process has finished unexpectedly and was restarted.");

    logError(msg);
}

void BackendCommunicator::logError(const QString &text)
{
    const QString textWithTimestamp = QDateTime::currentDateTime().toString(Qt::ISODate)
            + ' ' + text;
    Core::MessageManager::write(textWithTimestamp, Core::MessageManager::Flash);
    qWarning("%s", qPrintable(textWithTimestamp));
}

void BackendCommunicator::initializeBackendWithCurrentData()
{
    projectPartsUpdatedForFallback();
    projectPartsUpdatedForCurrentProjects();
    unsavedFilesUpdatedForUiHeaders();
    restoreCppEditorDocuments();
    documentVisibilityChanged();
}

void BackendCommunicator::documentsOpened(const FileContainers &fileContainers)
{
    const DocumentsOpenedMessage message(fileContainers,
                                         currentCppEditorDocumentFilePath(),
                                         visibleCppEditorDocumentsFilePaths());
    m_sender->documentsOpened(message);
}

void BackendCommunicator::documentsChanged(const FileContainers &fileContainers)
{
    const DocumentsChangedMessage message(fileContainers);
    m_sender->documentsChanged(message);
}

void BackendCommunicator::documentsClosed(const FileContainers &fileContainers)
{
    const DocumentsClosedMessage message(fileContainers);
    m_sender->documentsClosed(message);
}

void BackendCommunicator::projectPartsUpdated(
        const ProjectPartContainers &projectPartContainers)
{
    const ProjectPartsUpdatedMessage message(projectPartContainers);
    m_sender->projectPartsUpdated(message);
}

void BackendCommunicator::projectPartsRemoved(const QStringList &projectPartIds)
{
    const ProjectPartsRemovedMessage message((Utf8StringVector(projectPartIds)));
    m_sender->projectPartsRemoved(message);
}

void BackendCommunicator::unsavedFilesUpdated(const FileContainers &fileContainers)
{
    const UnsavedFilesUpdatedMessage message(fileContainers);
    m_sender->unsavedFilesUpdated(message);
}

void BackendCommunicator::unsavedFilesRemoved(const FileContainers &fileContainers)
{
    const UnsavedFilesRemovedMessage message(fileContainers);
    m_sender->unsavedFilesRemoved(message);
}

void BackendCommunicator::requestCompletions(ClangCompletionAssistProcessor *assistProcessor,
                                             const QString &filePath,
                                             quint32 line,
                                             quint32 column,
                                             const QString &projectFilePath,
                                             qint32 funcNameStartLine,
                                             qint32 funcNameStartColumn)
{
    const RequestCompletionsMessage message(filePath,
                                            line,
                                            column,
                                            projectFilePath,
                                            funcNameStartLine,
                                            funcNameStartColumn);
    m_sender->requestCompletions(message);
    m_receiver.addExpectedCompletionsMessage(message.ticketNumber, assistProcessor);
}

} // namespace Internal
} // namespace ClangCodeModel
