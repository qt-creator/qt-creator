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

#include "clangbackendipcintegration.h"

#include "clangcompletionassistprocessor.h"
#include "clangeditordocumentprocessor.h"
#include "clangmodelmanagersupport.h"
#include "clangutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <cpptools/abstracteditorsupport.h>
#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsbridge.h>
#include <cpptools/editordocumenthandle.h>
#include <cpptools/projectinfo.h>

#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/texteditor.h>


#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <clangbackendipc/cmbcodecompletedmessage.h>
#include <clangbackendipc/cmbcompletecodemessage.h>
#include <clangbackendipc/cmbechomessage.h>
#include <clangbackendipc/cmbendmessage.h>
#include <clangbackendipc/cmbregistertranslationunitsforeditormessage.h>
#include <clangbackendipc/cmbregisterprojectsforeditormessage.h>
#include <clangbackendipc/cmbunregistertranslationunitsforeditormessage.h>
#include <clangbackendipc/cmbunregisterprojectsforeditormessage.h>
#include <clangbackendipc/documentannotationschangedmessage.h>
#include <clangbackendipc/registerunsavedfilesforeditormessage.h>
#include <clangbackendipc/requestdocumentannotations.h>
#include <clangbackendipc/filecontainer.h>
#include <clangbackendipc/projectpartsdonotexistmessage.h>
#include <clangbackendipc/translationunitdoesnotexistmessage.h>
#include <clangbackendipc/unregisterunsavedfilesforeditormessage.h>
#include <clangbackendipc/updatetranslationunitsforeditormessage.h>
#include <clangbackendipc/updatevisibletranslationunitsmessage.h>

#include <cplusplus/Icons.h>

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QProcess>

static Q_LOGGING_CATEGORY(log, "qtc.clangcodemodel.ipc")

using namespace CPlusPlus;
using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace ClangBackEnd;
using namespace TextEditor;

static QString backendProcessPath()
{
    return Core::ICore::libexecPath()
            + QStringLiteral("/clangbackend")
            + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

static bool printAliveMessageHelper()
{
    const bool print = qEnvironmentVariableIntValue("QTC_CLANG_FORCE_VERBOSE_ALIVE");
    if (!print) {
        qCDebug(log) << "Hint: AliveMessage will not be printed. "
                        "Force it by setting QTC_CLANG_FORCE_VERBOSE_ALIVE=1.";
    }

    return print;
}

static bool printAliveMessage()
{
    static bool print = log().isDebugEnabled() ? printAliveMessageHelper() : false;
    return print;
}

IpcReceiver::IpcReceiver()
    : m_printAliveMessage(printAliveMessage())
{
}

IpcReceiver::~IpcReceiver()
{
    deleteAndClearWaitingAssistProcessors();
}

void IpcReceiver::setAliveHandler(const IpcReceiver::AliveHandler &handler)
{
    m_aliveHandler = handler;
}

void IpcReceiver::addExpectedCodeCompletedMessage(
        quint64 ticket,
        ClangCompletionAssistProcessor *processor)
{
    QTC_ASSERT(processor, return);
    QTC_CHECK(!m_assistProcessorsTable.contains(ticket));
    m_assistProcessorsTable.insert(ticket, processor);
}

void IpcReceiver::deleteAndClearWaitingAssistProcessors()
{
    qDeleteAll(m_assistProcessorsTable.begin(), m_assistProcessorsTable.end());
    m_assistProcessorsTable.clear();
}

void IpcReceiver::deleteProcessorsOfEditorWidget(TextEditor::TextEditorWidget *textEditorWidget)
{
    QMutableHashIterator<quint64, ClangCompletionAssistProcessor *> it(m_assistProcessorsTable);
    while (it.hasNext()) {
        it.next();
        ClangCompletionAssistProcessor *assistProcessor = it.value();
        if (assistProcessor->textEditorWidget() == textEditorWidget) {
            delete assistProcessor;
            it.remove();
        }
    }
}

bool IpcReceiver::isExpectingCodeCompletedMessage() const
{
    return !m_assistProcessorsTable.isEmpty();
}

void IpcReceiver::alive()
{
    if (m_printAliveMessage)
        qCDebug(log) << "<<< AliveMessage";
    QTC_ASSERT(m_aliveHandler, return);
    m_aliveHandler();
}

void IpcReceiver::echo(const EchoMessage &message)
{
    qCDebug(log) << "<<<" << message;
}

void IpcReceiver::codeCompleted(const CodeCompletedMessage &message)
{
    qCDebug(log) << "<<< CodeCompletedMessage with" << message.codeCompletions().size() << "items";

    const quint64 ticket = message.ticketNumber();
    QScopedPointer<ClangCompletionAssistProcessor> processor(m_assistProcessorsTable.take(ticket));
    if (processor) {
        const bool finished = processor->handleAvailableAsyncCompletions(
                                            message.codeCompletions(),
                                            message.neededCorrection());
        if (!finished)
            processor.take();
    }
}

void IpcReceiver::documentAnnotationsChanged(const DocumentAnnotationsChangedMessage &message)
{
    qCDebug(log) << "<<< DocumentAnnotationsChangedMessage with"
                 << message.diagnostics().size() << "diagnostics"
                 << message.highlightingMarks().size() << "highlighting marks"
                 << message.skippedPreprocessorRanges().size() << "skipped preprocessor ranges";

    auto processor = ClangEditorDocumentProcessor::get(message.fileContainer().filePath());

    if (processor) {
        const QString projectPartId = message.fileContainer().projectPartId();
        const QString filePath = message.fileContainer().filePath();
        const QString documentProjectPartId = CppTools::CppToolsBridge::projectPartIdForFile(filePath);
        if (projectPartId == documentProjectPartId) {
            const quint32 documentRevision = message.fileContainer().documentRevision();
            processor->updateCodeWarnings(message.diagnostics(),
                                          message.firstHeaderErrorDiagnostic(),
                                          documentRevision);
            processor->updateHighlighting(message.highlightingMarks(),
                                          message.skippedPreprocessorRanges(),
                                          documentRevision);
        }
    }
}

class IpcSender : public IpcSenderInterface
{
public:
    IpcSender(ClangBackEnd::ClangCodeModelConnectionClient &connectionClient)
        : m_connection(connectionClient)
    {}

    void end() override;
    void registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message) override;
    void updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message) override;
    void unregisterTranslationUnitsForEditor(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &message) override;
    void registerProjectPartsForEditor(const ClangBackEnd::RegisterProjectPartsForEditorMessage &message) override;
    void unregisterProjectPartsForEditor(const ClangBackEnd::UnregisterProjectPartsForEditorMessage &message) override;
    void registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message) override;
    void unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message) override;
    void completeCode(const ClangBackEnd::CompleteCodeMessage &message) override;
    void requestDocumentAnnotations(const ClangBackEnd::RequestDocumentAnnotationsMessage &message) override;
    void updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message) override;

private:
    ClangBackEnd::ClangCodeModelConnectionClient &m_connection;
};

void IpcSender::end()
{
     QTC_CHECK(m_connection.isConnected());
     qCDebug(log) << ">>>" << ClangBackEnd::EndMessage();
     m_connection.sendEndMessage();
}

void IpcSender::registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message)
{
     QTC_CHECK(m_connection.isConnected());
     qCDebug(log) << ">>>" << message;
     m_connection.serverProxy().registerTranslationUnitsForEditor(message);
}

void IpcSender::updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message)
{
    QTC_CHECK(m_connection.isConnected());
    qCDebug(log) << ">>>" << message;
    m_connection.serverProxy().updateTranslationUnitsForEditor(message);
}

void IpcSender::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
     QTC_CHECK(m_connection.isConnected());
     qCDebug(log) << ">>>" << message;
     m_connection.serverProxy().unregisterTranslationUnitsForEditor(message);
}

void IpcSender::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
     QTC_CHECK(m_connection.isConnected());
     qCDebug(log) << ">>>" << message;
     m_connection.serverProxy().registerProjectPartsForEditor(message);
}

void IpcSender::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
     QTC_CHECK(m_connection.isConnected());
     qCDebug(log) << ">>>" << message;
     m_connection.serverProxy().unregisterProjectPartsForEditor(message);
}

void IpcSender::registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message)
{
    QTC_CHECK(m_connection.isConnected());
    qCDebug(log) << ">>>" << message;
    m_connection.serverProxy().registerUnsavedFilesForEditor(message);
}

void IpcSender::unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message)
{
    QTC_CHECK(m_connection.isConnected());
    qCDebug(log) << ">>>" << message;
    m_connection.serverProxy().unregisterUnsavedFilesForEditor(message);
}

void IpcSender::completeCode(const CompleteCodeMessage &message)
{
     QTC_CHECK(m_connection.isConnected());
     qCDebug(log) << ">>>" << message;
     m_connection.serverProxy().completeCode(message);
}

void IpcSender::requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message)
{
    QTC_CHECK(m_connection.isConnected());
    qCDebug(log) << ">>>" << message;
    m_connection.serverProxy().requestDocumentAnnotations(message);
}

void IpcSender::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    QTC_CHECK(m_connection.isConnected());
    qCDebug(log) << ">>>" << message;
    m_connection.serverProxy().updateVisibleTranslationUnits(message);
}

class DummyIpcSender : public IpcSenderInterface
{
public:
    void end() override {}
    void registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &) override {}
    void updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &) override {}
    void unregisterTranslationUnitsForEditor(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &) override {}
    void registerProjectPartsForEditor(const ClangBackEnd::RegisterProjectPartsForEditorMessage &) override {}
    void unregisterProjectPartsForEditor(const ClangBackEnd::UnregisterProjectPartsForEditorMessage &) override {}
    void registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &) override {}
    void unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &) override {}
    void completeCode(const ClangBackEnd::CompleteCodeMessage &) override {}
    void requestDocumentAnnotations(const ClangBackEnd::RequestDocumentAnnotationsMessage &) override {}
    void updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &) override {}
};

enum { backEndStartTimeOutInMs = 10000 };

IpcCommunicator::IpcCommunicator()
    : m_connection(&m_ipcReceiver)
    , m_ipcSender(new DummyIpcSender)
{
    m_backendStartTimeOut.setSingleShot(true);
    connect(&m_backendStartTimeOut, &QTimer::timeout,
            this, &IpcCommunicator::logStartTimeOut);

    m_ipcReceiver.setAliveHandler([this]() { m_connection.resetProcessAliveTimer(); });

    connect(Core::EditorManager::instance(), &Core::EditorManager::editorAboutToClose,
            this, &IpcCommunicator::onEditorAboutToClose);
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
            this, &IpcCommunicator::setupDummySender);

    initializeBackend();
}

IpcCommunicator::~IpcCommunicator()
{
    disconnect(&m_connection, 0, this, 0);
}

void IpcCommunicator::initializeBackend()
{
    const QString clangBackEndProcessPath = backendProcessPath();
    if (!QFileInfo(clangBackEndProcessPath).exists()) {
        logExecutableDoesNotExist();
        return;
    }
    qCDebug(log) << "Starting" << clangBackEndProcessPath;

    m_connection.setProcessAliveTimerInterval(30 * 1000);
    m_connection.setProcessPath(clangBackEndProcessPath);

    connect(&m_connection, &ConnectionClient::connectedToLocalSocket,
            this, &IpcCommunicator::onConnectedToBackend);
    connect(&m_connection, &ConnectionClient::disconnectedFromLocalSocket,
            this, &IpcCommunicator::setupDummySender);

    m_connection.startProcessAndConnectToServerAsynchronously();
    m_backendStartTimeOut.start(backEndStartTimeOutInMs);
}

static QStringList projectPartOptions(const CppTools::ProjectPart::Ptr &projectPart)
{
    const QStringList options = ClangCodeModel::Utils::createClangOptions(projectPart,
        CppTools::ProjectFile::Unsupported); // No language option

    return options;
}

static ClangBackEnd::ProjectPartContainer toProjectPartContainer(
        const CppTools::ProjectPart::Ptr &projectPart)
{
    const QStringList options = projectPartOptions(projectPart);

    return ClangBackEnd::ProjectPartContainer(projectPart->id(), Utf8StringVector(options));
}

static QVector<ClangBackEnd::ProjectPartContainer> toProjectPartContainers(
        const QVector<CppTools::ProjectPart::Ptr> projectParts)
{
    QVector<ClangBackEnd::ProjectPartContainer> projectPartContainers;
    projectPartContainers.reserve(projectParts.size());

    foreach (const CppTools::ProjectPart::Ptr &projectPart, projectParts)
        projectPartContainers << toProjectPartContainer(projectPart);

    return projectPartContainers;
}

void IpcCommunicator::registerFallbackProjectPart()
{
    const auto projectPart = CppTools::CppModelManager::instance()->fallbackProjectPart();
    const auto projectPartContainer = toProjectPartContainer(projectPart);

    registerProjectPartsForEditor({projectPartContainer});
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

void IpcCommunicator::updateTranslationUnitVisiblity()
{
    updateTranslationUnitVisiblity(currentCppEditorDocumentFilePath(), visibleCppEditorDocumentsFilePaths());
}

bool IpcCommunicator::isNotWaitingForCompletion() const
{
    return !m_ipcReceiver.isExpectingCodeCompletedMessage();
}

void IpcCommunicator::updateTranslationUnitVisiblity(const Utf8String &currentEditorFilePath,
                                                     const Utf8StringVector &visibleEditorsFilePaths)
{
    const UpdateVisibleTranslationUnitsMessage message(currentEditorFilePath, visibleEditorsFilePaths);
    m_ipcSender->updateVisibleTranslationUnits(message);
}

void IpcCommunicator::registerCurrentProjectParts()
{
    using namespace CppTools;

    const QList<ProjectInfo> projectInfos = CppModelManager::instance()->projectInfos();
    foreach (const ProjectInfo &projectInfo, projectInfos)
        registerProjectsParts(projectInfo.projectParts());
}

void IpcCommunicator::restoreCppEditorDocuments()
{
    resetCppEditorDocumentProcessors();
    registerVisibleCppEditorDocumentAndMarkInvisibleDirty();
}

void IpcCommunicator::resetCppEditorDocumentProcessors()
{
    using namespace CppTools;

    const auto cppEditorDocuments = CppModelManager::instance()->cppEditorDocuments();
    foreach (CppEditorDocumentHandle *cppEditorDocument, cppEditorDocuments)
        cppEditorDocument->resetProcessor();
}

void IpcCommunicator::registerVisibleCppEditorDocumentAndMarkInvisibleDirty()
{
    CppTools::CppModelManager::instance()->updateCppEditorDocuments();
}

void IpcCommunicator::registerCurrentCodeModelUiHeaders()
{
    using namespace CppTools;

    const auto editorSupports = CppModelManager::instance()->abstractEditorSupports();
    foreach (const AbstractEditorSupport *es, editorSupports) {
        const QString mappedPath
                = ModelManagerSupportClang::instance()->dummyUiHeaderOnDiskPath(es->fileName());
        updateUnsavedFile(mappedPath, es->contents(), es->revision());
    }
}

void IpcCommunicator::registerProjectsParts(const QVector<CppTools::ProjectPart::Ptr> projectParts)
{
    const auto projectPartContainers = toProjectPartContainers(projectParts);
    registerProjectPartsForEditor(projectPartContainers);
}

void IpcCommunicator::updateTranslationUnitFromCppEditorDocument(const QString &filePath)
{
    const CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath);

    updateTranslationUnit(filePath, document->contents(), document->revision());
}

void IpcCommunicator::updateUnsavedFileFromCppEditorDocument(const QString &filePath)
{
    const CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath);

    updateUnsavedFile(filePath, document->contents(), document->revision());
}

namespace {


}

void IpcCommunicator::updateTranslationUnit(const QString &filePath,
                                            const QByteArray &contents,
                                            uint documentRevision)
{
    const bool hasUnsavedContent = true;

    updateTranslationUnitsForEditor({{filePath,
                                      Utf8String(),
                                      Utf8String::fromByteArray(contents),
                                      hasUnsavedContent,
                                      documentRevision}});
}

void IpcCommunicator::updateUnsavedFile(const QString &filePath, const QByteArray &contents, uint documentRevision)
{
    const bool hasUnsavedContent = true;

    // TODO: Send new only if changed
    registerUnsavedFilesForEditor({{filePath,
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

void IpcCommunicator::updateTranslationUnitWithRevisionCheck(const FileContainer &fileContainer)
{
    if (documentHasChanged(fileContainer.filePath(), fileContainer.documentRevision())) {
        updateTranslationUnitsForEditor({fileContainer});
        setLastSentDocumentRevision(fileContainer.filePath(),
                                    fileContainer.documentRevision());
    }
}

void IpcCommunicator::requestDocumentAnnotations(const FileContainer &fileContainer)
{
    const RequestDocumentAnnotationsMessage message(fileContainer);
    m_ipcSender->requestDocumentAnnotations(message);
}

void IpcCommunicator::updateTranslationUnitWithRevisionCheck(Core::IDocument *document)
{
    const auto textDocument = qobject_cast<TextDocument*>(document);
    const auto filePath = textDocument->filePath().toString();
    const QString projectPartId = CppTools::CppToolsBridge::projectPartIdForFile(filePath);

    updateTranslationUnitWithRevisionCheck(FileContainer(filePath,
                                                         projectPartId,
                                                         Utf8StringVector(),
                                                         textDocument->document()->revision()));
}

void IpcCommunicator::updateChangeContentStartPosition(const QString &filePath, int position)
{
    if (CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath))
        document->sendTracker().applyContentChange(position);
}

void IpcCommunicator::updateTranslationUnitIfNotCurrentDocument(Core::IDocument *document)
{
    QTC_ASSERT(document, return);
    if (Core::EditorManager::currentDocument() != document)
        updateTranslationUnit(document);
}

void IpcCommunicator::updateTranslationUnit(Core::IDocument *document)
{
    updateTranslationUnitFromCppEditorDocument(document->filePath().toString());
}

void IpcCommunicator::updateUnsavedFile(Core::IDocument *document)
{
    QTC_ASSERT(document, return);

     updateUnsavedFileFromCppEditorDocument(document->filePath().toString());
}

void IpcCommunicator::onConnectedToBackend()
{
    m_backendStartTimeOut.stop();

    ++m_connectedCount;
    if (m_connectedCount > 1)
        logRestartedDueToUnexpectedFinish();

    m_ipcReceiver.deleteAndClearWaitingAssistProcessors();
    m_ipcSender.reset(new IpcSender(m_connection));

    initializeBackendWithCurrentData();
}

void IpcCommunicator::onEditorAboutToClose(Core::IEditor *editor)
{
    if (auto *textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor))
        m_ipcReceiver.deleteProcessorsOfEditorWidget(textEditor->editorWidget());
}

void IpcCommunicator::setupDummySender()
{
    m_ipcSender.reset(new DummyIpcSender);
}

void IpcCommunicator::logExecutableDoesNotExist()
{
    const QString msg
        = tr("Clang Code Model: Error: "
             "The clangbackend executable \"%1\" does not exist.")
                .arg(QDir::toNativeSeparators(backendProcessPath()));

    logError(msg);
}

void IpcCommunicator::logStartTimeOut()
{
    const QString msg
        = tr("Clang Code Model: Error: "
             "The clangbackend executable \"%1\" could not be started (timeout after %2ms).")
                .arg(QDir::toNativeSeparators(backendProcessPath()))
                .arg(backEndStartTimeOutInMs);

    logError(msg);
}

void IpcCommunicator::logRestartedDueToUnexpectedFinish()
{
    const QString msg
        = tr("Clang Code Model: Error: "
             "The clangbackend process has finished unexpectedly and was restarted.");

    logError(msg);
}

void IpcCommunicator::logError(const QString &text)
{
    const QString textWithTimestamp = QDateTime::currentDateTime().toString(Qt::ISODate)
            + ' ' + text;
    Core::MessageManager::write(textWithTimestamp, Core::MessageManager::Flash);
    qWarning("%s", qPrintable(textWithTimestamp));
}

void IpcCommunicator::initializeBackendWithCurrentData()
{
    registerFallbackProjectPart();
    registerCurrentProjectParts();
    registerCurrentCodeModelUiHeaders();
    restoreCppEditorDocuments();
    updateTranslationUnitVisiblity();

    emit backendReinitialized();
}

IpcSenderInterface *IpcCommunicator::setIpcSender(IpcSenderInterface *ipcSender)
{
    IpcSenderInterface *previousMessageSender = m_ipcSender.take();
    m_ipcSender.reset(ipcSender);
    return previousMessageSender;
}

void IpcCommunicator::killBackendProcess()
{
    m_connection.processForTestOnly()->kill();
}

void IpcCommunicator::registerTranslationUnitsForEditor(const FileContainers &fileContainers)
{
    const RegisterTranslationUnitForEditorMessage message(fileContainers,
                                                          currentCppEditorDocumentFilePath(),
                                                          visibleCppEditorDocumentsFilePaths());
    m_ipcSender->registerTranslationUnitsForEditor(message);
}

void IpcCommunicator::updateTranslationUnitsForEditor(const IpcCommunicator::FileContainers &fileContainers)
{
    const UpdateTranslationUnitsForEditorMessage message(fileContainers);
    m_ipcSender->updateTranslationUnitsForEditor(message);
}

void IpcCommunicator::unregisterTranslationUnitsForEditor(const FileContainers &fileContainers)
{
    const UnregisterTranslationUnitsForEditorMessage message(fileContainers);
    m_ipcSender->unregisterTranslationUnitsForEditor(message);
}

void IpcCommunicator::registerProjectPartsForEditor(
        const ProjectPartContainers &projectPartContainers)
{
    const RegisterProjectPartsForEditorMessage message(projectPartContainers);
    m_ipcSender->registerProjectPartsForEditor(message);
}

void IpcCommunicator::unregisterProjectPartsForEditor(const QStringList &projectPartIds)
{
    const UnregisterProjectPartsForEditorMessage message((Utf8StringVector(projectPartIds)));
    m_ipcSender->unregisterProjectPartsForEditor(message);
}

void IpcCommunicator::registerUnsavedFilesForEditor(const IpcCommunicator::FileContainers &fileContainers)
{
    const RegisterUnsavedFilesForEditorMessage message(fileContainers);
    m_ipcSender->registerUnsavedFilesForEditor(message);
}

void IpcCommunicator::unregisterUnsavedFilesForEditor(const IpcCommunicator::FileContainers &fileContainers)
{
    const UnregisterUnsavedFilesForEditorMessage message(fileContainers);
    m_ipcSender->unregisterUnsavedFilesForEditor(message);
}

void IpcCommunicator::completeCode(ClangCompletionAssistProcessor *assistProcessor,
                                   const QString &filePath,
                                   quint32 line,
                                   quint32 column,
                                   const QString &projectFilePath)
{
    const CompleteCodeMessage message(filePath, line, column, projectFilePath);
    m_ipcSender->completeCode(message);
    m_ipcReceiver.addExpectedCodeCompletedMessage(message.ticketNumber(), assistProcessor);
}
