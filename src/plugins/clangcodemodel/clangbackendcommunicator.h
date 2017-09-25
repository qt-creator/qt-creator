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

#include "clangbackendreceiver.h"
#include "clangbackendsender.h"

#include <cpptools/projectpart.h>

#include <clangsupport/clangcodemodelconnectionclient.h>
#include <clangsupport/filecontainer.h>
#include <clangsupport/projectpartcontainer.h>

#include <QFuture>
#include <QObject>
#include <QVector>
#include <QTimer>

namespace Core {
class IEditor;
class IDocument;
}

namespace ClangCodeModel {
namespace Internal {

class ClangCompletionAssistProcessor;

class BackendCommunicator : public QObject
{
    Q_OBJECT

public:
    using FileContainer = ClangBackEnd::FileContainer;
    using FileContainers = QVector<ClangBackEnd::FileContainer>;
    using ProjectPartContainers = QVector<ClangBackEnd::ProjectPartContainer>;

public:
    BackendCommunicator();
    ~BackendCommunicator();

    void registerTranslationUnitsForEditor(const FileContainers &fileContainers);
    void updateTranslationUnitsForEditor(const FileContainers &fileContainers);
    void unregisterTranslationUnitsForEditor(const FileContainers &fileContainers);
    void registerProjectPartsForEditor(const ProjectPartContainers &projectPartContainers);
    void unregisterProjectPartsForEditor(const QStringList &projectPartIds);
    void registerUnsavedFilesForEditor(const FileContainers &fileContainers);
    void unregisterUnsavedFilesForEditor(const FileContainers &fileContainers);
    void requestDocumentAnnotations(const ClangBackEnd::FileContainer &fileContainer);
    QFuture<CppTools::CursorInfo> requestReferences(
            const FileContainer &fileContainer,
            quint32 line,
            quint32 column,
            QTextDocument *textDocument,
            const CppTools::SemanticInfo::LocalUseMap &localUses);
    QFuture<CppTools::SymbolInfo> requestFollowSymbol(const FileContainer &curFileContainer,
                                                      const QVector<Utf8String> &dependentFiles,
                                                      quint32 line,
                                                      quint32 column);
    void completeCode(ClangCompletionAssistProcessor *assistProcessor, const QString &filePath,
                      quint32 line,
                      quint32 column,
                      const QString &projectFilePath,
                      qint32 funcNameStartLine = -1,
                      qint32 funcNameStartColumn = -1);

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
    BackendSender *setBackendSender(BackendSender *sender);
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
    BackendReceiver m_receiver;
    ClangBackEnd::ClangCodeModelConnectionClient m_connection;
    QTimer m_backendStartTimeOut;
    QScopedPointer<BackendSender> m_sender;
    int m_connectedCount = 0;
};

} // namespace Internal
} // namespace ClangCodeModel
