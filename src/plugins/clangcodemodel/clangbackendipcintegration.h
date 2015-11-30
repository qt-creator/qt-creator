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

#ifndef CLANGCODEMODEL_INTERNAL_CLANGBACKENDIPCINTEGRATION_H
#define CLANGCODEMODEL_INTERNAL_CLANGBACKENDIPCINTEGRATION_H

#include <cpptools/cppprojects.h>

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

private:
    void alive() override;
    void echo(const ClangBackEnd::EchoMessage &message) override;
    void codeCompleted(const ClangBackEnd::CodeCompletedMessage &message) override;
    void diagnosticsChanged(const ClangBackEnd::DiagnosticsChangedMessage &message) override;

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
    void requestDiagnostics(const ClangBackEnd::FileContainer &fileContainer);
    void requestDiagnostics(Core::IDocument *document);
    void updateChangeContentStartPosition(const QString &filePath, int position);

    void registerFallbackProjectPart();

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
    void registerCurrentCppEditorDocuments();
    void registerCurrentCodeModelUiHeaders();

    void onBackendRestarted();
    void onEditorAboutToClose(Core::IEditor *editor);
    void onCoreAboutToClose();

private:
    IpcReceiver m_ipcReceiver;
    ClangBackEnd::ConnectionClient m_connection;
    QScopedPointer<IpcSenderInterface> m_ipcSender;

    SendMode m_sendMode = RespectSendRequests;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CLANGCODEMODEL_INTERNAL_CLANGBACKENDIPCINTEGRATION_H
