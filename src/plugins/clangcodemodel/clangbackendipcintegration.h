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

#ifndef CLANGCODEMODEL_INTERNAL_CLANGBACKENDIPCINTEGRATION_H
#define CLANGCODEMODEL_INTERNAL_CLANGBACKENDIPCINTEGRATION_H

#include <cpptools/projectpart.h>

#include <clangbackendipc/connectionclient.h>
#include <clangbackendipc/filecontainer.h>
#include <clangbackendipc/ipcclientinterface.h>
#include <clangbackendipc/projectpartcontainer.h>

#include <QObject>
#include <QSharedPointer>
#include <QVector>

namespace Core {
class IEditor;
class IDocument;
}

namespace ClangBackEnd {
class DiagnosticsChangedMessage;
}

namespace TextEditor {
class TextEditorWidget;
class TextDocument;
}

namespace ClangCodeModel {
namespace Internal {

class ModelManagerSupportClang;

class ClangCompletionAssistProcessor;

class IpcReceiver : public ClangBackEnd::IpcClientInterface
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
    void diagnosticsChanged(const ClangBackEnd::DiagnosticsChangedMessage &message) override;
    void highlightingChanged(const ClangBackEnd::HighlightingChangedMessage &message) override;

    void translationUnitDoesNotExist(const ClangBackEnd::TranslationUnitDoesNotExistMessage &message) override;
    void projectPartsDoNotExist(const ClangBackEnd::ProjectPartsDoNotExistMessage &message) override;

private:
    AliveHandler m_aliveHandler;
    QHash<quint64, ClangCompletionAssistProcessor *> m_assistProcessorsTable;
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
    virtual void requestDiagnostics(const ClangBackEnd::RequestDiagnosticsMessage &message) = 0;
    virtual void requestHighlighting(const ClangBackEnd::RequestHighlightingMessage &message) = 0;
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

    void registerTranslationUnitsForEditor(const FileContainers &fileContainers);
    void updateTranslationUnitsForEditor(const FileContainers &fileContainers);
    void unregisterTranslationUnitsForEditor(const FileContainers &fileContainers);
    void registerProjectPartsForEditor(const ProjectPartContainers &projectPartContainers);
    void unregisterProjectPartsForEditor(const QStringList &projectPartIds);
    void registerUnsavedFilesForEditor(const FileContainers &fileContainers);
    void unregisterUnsavedFilesForEditor(const FileContainers &fileContainers);
    void requestDiagnostics(const ClangBackEnd::FileContainer &fileContainer);
    void requestHighlighting(const ClangBackEnd::FileContainer &fileContainer);
    void completeCode(ClangCompletionAssistProcessor *assistProcessor, const QString &filePath,
                      quint32 line,
                      quint32 column,
                      const QString &projectFilePath);

    void registerProjectsParts(const QList<CppTools::ProjectPart::Ptr> projectParts);

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
    enum SendMode { RespectSendRequests, IgnoreSendRequests };

    void initializeBackend();
    void initializeBackendWithCurrentData();
    void registerCurrentProjectParts();
    void restoreCppEditorDocuments();
    void resetCppEditorDocumentProcessors();
    void registerVisibleCppEditorDocumentAndMarkInvisibleDirty();
    void registerCurrentCodeModelUiHeaders();


    void onBackendRestarted();
    void onEditorAboutToClose(Core::IEditor *editor);
    void onCoreAboutToClose();

    void updateTranslationUnitVisiblity(const Utf8String &currentEditorFilePath,
                                        const Utf8StringVector &visibleEditorsFilePaths);

private:
    IpcReceiver m_ipcReceiver;
    ClangBackEnd::ConnectionClient m_connection;
    QScopedPointer<IpcSenderInterface> m_ipcSender;

    SendMode m_sendMode = RespectSendRequests;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CLANGCODEMODEL_INTERNAL_CLANGBACKENDIPCINTEGRATION_H
