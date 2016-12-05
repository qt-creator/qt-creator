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

#pragma once

#include <cpptools/projectpart.h>

#include <clangbackendipc/clangcodemodelconnectionclient.h>
#include <clangbackendipc/filecontainer.h>
#include <clangbackendipc/clangcodemodelclientinterface.h>
#include <clangbackendipc/projectpartcontainer.h>

#include <QObject>
#include <QSharedPointer>
#include <QVector>

#include <functional>

namespace Core {
class IEditor;
class IDocument;
}

namespace ClangBackEnd {
class DocumentAnnotationsChangedMessage;
}

namespace TextEditor {
class TextEditorWidget;
class TextDocument;
}

namespace ClangCodeModel {
namespace Internal {

class ModelManagerSupportClang;

class ClangCompletionAssistProcessor;

class IpcReceiver : public ClangBackEnd::ClangCodeModelClientInterface
{
public:
    IpcReceiver();
    ~IpcReceiver();

    using AliveHandler = std::function<void ()>;
    void setAliveHandler(const AliveHandler &handler);

    void addExpectedCodeCompletedMessage(quint64 ticket, ClangCompletionAssistProcessor *processor);
    void deleteAndClearWaitingAssistProcessors();
    void deleteProcessorsOfEditorWidget(TextEditor::TextEditorWidget *textEditorWidget);

    bool isExpectingCodeCompletedMessage() const;

private:
    void alive() override;
    void echo(const ClangBackEnd::EchoMessage &message) override;
    void codeCompleted(const ClangBackEnd::CodeCompletedMessage &message) override;

    void documentAnnotationsChanged(const ClangBackEnd::DocumentAnnotationsChangedMessage &message) override;

    void translationUnitDoesNotExist(const ClangBackEnd::TranslationUnitDoesNotExistMessage &) override {}
    void projectPartsDoNotExist(const ClangBackEnd::ProjectPartsDoNotExistMessage &) override {}

private:
    AliveHandler m_aliveHandler;
    QHash<quint64, ClangCompletionAssistProcessor *> m_assistProcessorsTable;
    const bool m_printAliveMessage = false;
};

class IpcSenderInterface
{
public:
    virtual ~IpcSenderInterface() {}

    virtual void end() = 0;
    virtual void registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message) = 0;
    virtual void updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message) = 0;
    virtual void unregisterTranslationUnitsForEditor(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &message) = 0;
    virtual void registerProjectPartsForEditor(const ClangBackEnd::RegisterProjectPartsForEditorMessage &message) = 0;
    virtual void unregisterProjectPartsForEditor(const ClangBackEnd::UnregisterProjectPartsForEditorMessage &message) = 0;
    virtual void registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message) = 0;
    virtual void unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message) = 0;
    virtual void completeCode(const ClangBackEnd::CompleteCodeMessage &message) = 0;
    virtual void requestDocumentAnnotations(const ClangBackEnd::RequestDocumentAnnotationsMessage &message) = 0;
    virtual void updateVisibleTranslationUnits(const ClangBackEnd::UpdateVisibleTranslationUnitsMessage &message) = 0;
};

class IpcCommunicator : public QObject
{
    Q_OBJECT

public:
    using Ptr = QSharedPointer<IpcCommunicator>;
    using FileContainers = QVector<ClangBackEnd::FileContainer>;
    using ProjectPartContainers = QVector<ClangBackEnd::ProjectPartContainer>;

public:
    IpcCommunicator();
    ~IpcCommunicator();

    void registerTranslationUnitsForEditor(const FileContainers &fileContainers);
    void updateTranslationUnitsForEditor(const FileContainers &fileContainers);
    void unregisterTranslationUnitsForEditor(const FileContainers &fileContainers);
    void registerProjectPartsForEditor(const ProjectPartContainers &projectPartContainers);
    void unregisterProjectPartsForEditor(const QStringList &projectPartIds);
    void registerUnsavedFilesForEditor(const FileContainers &fileContainers);
    void unregisterUnsavedFilesForEditor(const FileContainers &fileContainers);
    void requestDocumentAnnotations(const ClangBackEnd::FileContainer &fileContainer);
    void completeCode(ClangCompletionAssistProcessor *assistProcessor, const QString &filePath,
                      quint32 line,
                      quint32 column,
                      const QString &projectFilePath);

    void registerProjectsParts(const QVector<CppTools::ProjectPart::Ptr> projectParts);

    void updateTranslationUnitIfNotCurrentDocument(Core::IDocument *document);
    void updateTranslationUnit(Core::IDocument *document);
    void updateUnsavedFile(Core::IDocument *document);
    void updateTranslationUnitFromCppEditorDocument(const QString &filePath);
    void updateUnsavedFileFromCppEditorDocument(const QString &filePath);
    void updateTranslationUnit(const QString &filePath, const QByteArray &contents, uint documentRevision);
    void updateUnsavedFile(const QString &filePath, const QByteArray &contents, uint documentRevision);
    void updateTranslationUnitWithRevisionCheck(const ClangBackEnd::FileContainer &fileContainer);
    void updateTranslationUnitWithRevisionCheck(Core::IDocument *document);
    void updateChangeContentStartPosition(const QString &filePath, int position);

    void registerFallbackProjectPart();
    void updateTranslationUnitVisiblity();

    bool isNotWaitingForCompletion() const;

public: // for tests
    IpcSenderInterface *setIpcSender(IpcSenderInterface *ipcSender);
    void killBackendProcess();

signals: // for tests
    void backendReinitialized();

private:
    void initializeBackend();
    void initializeBackendWithCurrentData();
    void registerCurrentProjectParts();
    void restoreCppEditorDocuments();
    void resetCppEditorDocumentProcessors();
    void registerVisibleCppEditorDocumentAndMarkInvisibleDirty();
    void registerCurrentCodeModelUiHeaders();

    void setupDummySender();

    void onConnectedToBackend();
    void onDisconnectedFromBackend();
    void onEditorAboutToClose(Core::IEditor *editor);

    void logExecutableDoesNotExist();
    void logRestartedDueToUnexpectedFinish();
    void logStartTimeOut();
    void logError(const QString &text);

    void updateTranslationUnitVisiblity(const Utf8String &currentEditorFilePath,
                                        const Utf8StringVector &visibleEditorsFilePaths);

private:
    IpcReceiver m_ipcReceiver;
    ClangBackEnd::ClangCodeModelConnectionClient m_connection;
    QTimer m_backendStartTimeOut;
    QScopedPointer<IpcSenderInterface> m_ipcSender;
    int m_connectedCount = 0;
};

} // namespace Internal
} // namespace ClangCodeModel
