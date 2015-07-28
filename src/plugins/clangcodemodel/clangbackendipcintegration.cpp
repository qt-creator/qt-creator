/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangbackendipcintegration.h"

#include "clangcompletionassistprocessor.h"
#include "clangmodelmanagersupport.h"
#include "clangutils.h"
#include "pchmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <cpptools/abstracteditorsupport.h>
#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/editordocumenthandle.h>

#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/texteditor.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <clangbackendipc/cmbcodecompletedcommand.h>
#include <clangbackendipc/cmbcompletecodecommand.h>
#include <clangbackendipc/cmbechocommand.h>
#include <clangbackendipc/cmbregistertranslationunitsforcodecompletioncommand.h>
#include <clangbackendipc/cmbregisterprojectsforcodecompletioncommand.h>
#include <clangbackendipc/cmbunregistertranslationunitsforcodecompletioncommand.h>
#include <clangbackendipc/cmbunregisterprojectsforcodecompletioncommand.h>
#include <clangbackendipc/cmbcommands.h>
#include <clangbackendipc/projectpartsdonotexistcommand.h>
#include <clangbackendipc/translationunitdoesnotexistcommand.h>

#include <cplusplus/Icons.h>

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QProcess>

static Q_LOGGING_CATEGORY(log, "qtc.clangcodemodel.ipc")

using namespace CPlusPlus;
using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace ClangBackEnd;
using namespace TextEditor;

namespace {

QString backendProcessPath()
{
    return Core::ICore::libexecPath()
            + QStringLiteral("/clangbackend")
            + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

} // anonymous namespace

IpcReceiver::IpcReceiver()
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

void IpcReceiver::addExpectedCodeCompletedCommand(
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

void IpcReceiver::alive()
{
    qCDebug(log) << "<<< AliveCommand";
    QTC_ASSERT(m_aliveHandler, return);
    m_aliveHandler();
}

void IpcReceiver::echo(const EchoCommand &command)
{
    qCDebug(log) << "<<<" << command;
}

void IpcReceiver::codeCompleted(const CodeCompletedCommand &command)
{
    qCDebug(log) << "<<< CodeCompletedCommand with" << command.codeCompletions().size() << "items";

    const quint64 ticket = command.ticketNumber();
    QScopedPointer<ClangCompletionAssistProcessor> processor(m_assistProcessorsTable.take(ticket));
    if (processor) {
        const bool finished = processor->handleAvailableAsyncCompletions(command.codeCompletions());
        if (!finished)
            processor.take();
    }
}

void IpcReceiver::translationUnitDoesNotExist(const TranslationUnitDoesNotExistCommand &command)
{
    QTC_CHECK(!"Got TranslationUnitDoesNotExistCommand");
    qCDebug(log) << "<<< ERROR:" << command;
}

void IpcReceiver::projectPartsDoNotExist(const ProjectPartsDoNotExistCommand &command)
{
    QTC_CHECK(!"Got ProjectPartsDoNotExistCommand");
    qCDebug(log) << "<<< ERROR:" << command;
}

class IpcSender : public IpcSenderInterface
{
public:
    IpcSender(ClangBackEnd::ConnectionClient &connectionClient)
        : m_connection(connectionClient)
    {}

    void end() override;
    void registerTranslationUnitsForCodeCompletion(const ClangBackEnd::RegisterTranslationUnitForCodeCompletionCommand &command) override;
    void unregisterTranslationUnitsForCodeCompletion(const ClangBackEnd::UnregisterTranslationUnitsForCodeCompletionCommand &command) override;
    void registerProjectPartsForCodeCompletion(const ClangBackEnd::RegisterProjectPartsForCodeCompletionCommand &command) override;
    void unregisterProjectPartsForCodeCompletion(const ClangBackEnd::UnregisterProjectPartsForCodeCompletionCommand &command) override;
    void completeCode(const ClangBackEnd::CompleteCodeCommand &command) override;

private:
    ClangBackEnd::ConnectionClient &m_connection;
};

void IpcSender::end()
{
     QTC_CHECK(m_connection.isConnected());
     m_connection.sendEndCommand();
}

void IpcSender::registerTranslationUnitsForCodeCompletion(const RegisterTranslationUnitForCodeCompletionCommand &command)
{
     QTC_CHECK(m_connection.isConnected());
     m_connection.serverProxy().registerTranslationUnitsForCodeCompletion(command);
}

void IpcSender::unregisterTranslationUnitsForCodeCompletion(const UnregisterTranslationUnitsForCodeCompletionCommand &command)
{
     QTC_CHECK(m_connection.isConnected());
     m_connection.serverProxy().unregisterTranslationUnitsForCodeCompletion(command);
}

void IpcSender::registerProjectPartsForCodeCompletion(const RegisterProjectPartsForCodeCompletionCommand &command)
{
     QTC_CHECK(m_connection.isConnected());
     m_connection.serverProxy().registerProjectPartsForCodeCompletion(command);
}

void IpcSender::unregisterProjectPartsForCodeCompletion(const UnregisterProjectPartsForCodeCompletionCommand &command)
{
     QTC_CHECK(m_connection.isConnected());
     m_connection.serverProxy().unregisterProjectPartsForCodeCompletion(command);
}

void IpcSender::completeCode(const CompleteCodeCommand &command)
{
     QTC_CHECK(m_connection.isConnected());
     m_connection.serverProxy().completeCode(command);
}

IpcCommunicator::IpcCommunicator()
    : m_connection(&m_ipcReceiver)
    , m_ipcSender(new IpcSender(m_connection))
{
    m_ipcReceiver.setAliveHandler([this]() { m_connection.resetProcessAliveTimer(); });

    connect(Core::EditorManager::instance(), &Core::EditorManager::editorAboutToClose,
            this, &IpcCommunicator::onEditorAboutToClose);
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
            this, &IpcCommunicator::onCoreAboutToClose);

    initializeBackend();
}

static bool areCommandsRegistered = false;

void IpcCommunicator::initializeBackend()
{
    if (!areCommandsRegistered) {
        areCommandsRegistered = true;
        Commands::registerCommands();
    }

    const QString clangBackEndProcessPath = backendProcessPath();
    qCDebug(log) << "Starting" << clangBackEndProcessPath;
    QTC_ASSERT(QFileInfo(clangBackEndProcessPath).exists(), return);

    m_connection.setProcessAliveTimerInterval(10 * 1000);
    m_connection.setProcessPath(clangBackEndProcessPath);

    connect(&m_connection, &ConnectionClient::processRestarted,
            this, &IpcCommunicator::onBackendRestarted);

    // TODO: Add a asynchron API to ConnectionClient, otherwise we might hang here
    if (m_connection.connectToServer())
        initializeBackendWithCurrentData();
}

void IpcCommunicator::registerEmptyProjectForProjectLessFiles()
{
    QTC_CHECK(m_connection.isConnected());
    registerProjectPartsForCodeCompletion({ClangBackEnd::ProjectPartContainer(
                                           Utf8String(),
                                           Utf8StringVector())});
}

void IpcCommunicator::registerCurrentProjectParts()
{
    using namespace CppTools;

    const QList<ProjectInfo> projectInfos = CppModelManager::instance()->projectInfos();
    foreach (const ProjectInfo &projectInfo, projectInfos)
        registerProjectsParts(projectInfo.projectParts());
}

void IpcCommunicator::registerCurrentUnsavedFiles()
{
    using namespace CppTools;

    const auto cppEditorDocuments = CppModelManager::instance()->cppEditorDocuments();
    foreach (const CppEditorDocumentHandle *cppEditorDocument, cppEditorDocuments) {
        if (cppEditorDocument->processor()->baseTextDocument()->isModified())
            updateUnsavedFileFromCppEditorDocument(cppEditorDocument->filePath());
    }

}

void IpcCommunicator::registerCurrrentCodeModelUiHeaders()
{
    using namespace CppTools;

    const auto editorSupports = CppModelManager::instance()->abstractEditorSupports();
    foreach (const AbstractEditorSupport *es, editorSupports)
        updateUnsavedFile(es->fileName(), es->contents());
}

static QStringList projectPartCommandLine(const CppTools::ProjectPart::Ptr &projectPart)
{
    QStringList options = ClangCodeModel::Utils::createClangOptions(projectPart,
        CppTools::ProjectFile::Unclassified); // No language option
    if (PchInfo::Ptr pchInfo = PchManager::instance()->pchInfo(projectPart))
        options += ClangCodeModel::Utils::createPCHInclusionOptions(pchInfo->fileName());
    return options;
}

static ClangBackEnd::ProjectPartContainer toProjectPartContainer(
        const CppTools::ProjectPart::Ptr &projectPart)
{
    const QStringList arguments = projectPartCommandLine(projectPart);
    return ClangBackEnd::ProjectPartContainer(projectPart->id(), Utf8StringVector(arguments));
}

static QVector<ClangBackEnd::ProjectPartContainer> toProjectPartContainers(
        const QList<CppTools::ProjectPart::Ptr> projectParts)
{
    QVector<ClangBackEnd::ProjectPartContainer> projectPartContainers;
    projectPartContainers.reserve(projectParts.size());
    foreach (const CppTools::ProjectPart::Ptr &projectPart, projectParts)
        projectPartContainers << toProjectPartContainer(projectPart);
    return projectPartContainers;
}

void IpcCommunicator::registerProjectsParts(const QList<CppTools::ProjectPart::Ptr> projectParts)
{
    const auto projectPartContainers = toProjectPartContainers(projectParts);
    registerProjectPartsForCodeCompletion(projectPartContainers);
}

void IpcCommunicator::updateUnsavedFileFromCppEditorDocument(const QString &filePath)
{
    const QByteArray unsavedContent = CppTools::CppModelManager::instance()
            ->cppEditorDocument(filePath)->contents();
    updateUnsavedFile(filePath, unsavedContent);
}

void IpcCommunicator::updateUnsavedFile(const QString &filePath, const QByteArray &contents)
{
    const QString projectPartId = Utils::projectPartIdForFile(filePath);
    const bool hasUnsavedContent = true;

    // TODO: Send new only if changed
    registerFilesForCodeCompletion({
        ClangBackEnd::FileContainer(filePath,
            projectPartId,
            Utf8String::fromByteArray(contents),
            hasUnsavedContent)
    });
}

void IpcCommunicator::updateUnsavedFileIfNotCurrentDocument(Core::IDocument *document)
{
    QTC_ASSERT(document, return);

    if (Core::EditorManager::currentDocument() != document)
        updateUnsavedFileFromCppEditorDocument(document->filePath().toString());
}

void IpcCommunicator::onBackendRestarted()
{
    qWarning("Clang back end finished unexpectedly, restarted.");
    qCDebug(log) << "Backend restarted, re-initializing with project data and unsaved files.";

    m_ipcReceiver.deleteAndClearWaitingAssistProcessors();
    initializeBackendWithCurrentData();
}

void IpcCommunicator::onEditorAboutToClose(Core::IEditor *editor)
{
    if (auto *textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor))
        m_ipcReceiver.deleteProcessorsOfEditorWidget(textEditor->editorWidget());
}

void IpcCommunicator::onCoreAboutToClose()
{
    m_sendMode = IgnoreSendRequests;
}

void IpcCommunicator::initializeBackendWithCurrentData()
{
    registerEmptyProjectForProjectLessFiles();
    registerCurrentProjectParts();
    registerCurrentUnsavedFiles();
    registerCurrrentCodeModelUiHeaders();

    emit backendReinitialized();
}

IpcSenderInterface *IpcCommunicator::setIpcSender(IpcSenderInterface *ipcSender)
{
    IpcSenderInterface *previousCommandSender = m_ipcSender.take();
    m_ipcSender.reset(ipcSender);
    return previousCommandSender;
}

void IpcCommunicator::killBackendProcess()
{
    m_connection.processForTestOnly()->kill();
}

void IpcCommunicator::registerFilesForCodeCompletion(const FileContainers &fileContainers)
{
    if (m_sendMode == IgnoreSendRequests)
        return;

    const RegisterTranslationUnitForCodeCompletionCommand command(fileContainers);
    qCDebug(log) << ">>>" << command;
    m_ipcSender->registerTranslationUnitsForCodeCompletion(command);
}

void IpcCommunicator::unregisterFilesForCodeCompletion(const FileContainers &fileContainers)
{
    if (m_sendMode == IgnoreSendRequests)
        return;

    const UnregisterTranslationUnitsForCodeCompletionCommand command(fileContainers);
    qCDebug(log) << ">>>" << command;
    m_ipcSender->unregisterTranslationUnitsForCodeCompletion(command);
}

void IpcCommunicator::registerProjectPartsForCodeCompletion(
        const ProjectPartContainers &projectPartContainers)
{
    if (m_sendMode == IgnoreSendRequests)
        return;

    const RegisterProjectPartsForCodeCompletionCommand command(projectPartContainers);
    qCDebug(log) << ">>>" << command;
    m_ipcSender->registerProjectPartsForCodeCompletion(command);
}

void IpcCommunicator::unregisterProjectPartsForCodeCompletion(const QStringList &projectPartIds)
{
    if (m_sendMode == IgnoreSendRequests)
        return;

    const UnregisterProjectPartsForCodeCompletionCommand command((Utf8StringVector(projectPartIds)));
    qCDebug(log) << ">>>" << command;
    m_ipcSender->unregisterProjectPartsForCodeCompletion(command);
}

void IpcCommunicator::completeCode(ClangCompletionAssistProcessor *assistProcessor,
                              const QString &filePath,
                              quint32 line,
                              quint32 column,
                              const QString &projectFilePath)
{
    if (m_sendMode == IgnoreSendRequests)
        return;

    const CompleteCodeCommand command(filePath, line, column, projectFilePath);
    qCDebug(log) << ">>>" << command;
    m_ipcSender->completeCode(command);
    m_ipcReceiver.addExpectedCodeCompletedCommand(command.ticketNumber(), assistProcessor);
}
