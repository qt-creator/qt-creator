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

#include <QFuture>
#include <QObject>
#include <QVector>
#include <QTimer>

namespace Core {
class IEditor;
class IDocument;
}

namespace TextEditor { class IAssistProcessor; }

namespace ClangCodeModel {
namespace Internal {

class ClangCompletionAssistProcessor;

class BackendCommunicator : public QObject
{
    Q_OBJECT

public:
    using FileContainer = ClangBackEnd::FileContainer;
    using FileContainers = QVector<ClangBackEnd::FileContainer>;
    using LocalUseMap = CppTools::SemanticInfo::LocalUseMap;

public:
    BackendCommunicator();
    ~BackendCommunicator() override;

    void documentsOpened(const FileContainers &fileContainers);
    void documentsChanged(Core::IDocument *document);
    void documentsChanged(const QString &filePath,
                          const QByteArray &contents,
                          uint documentRevision);
    void documentsChanged(const FileContainers &fileContainers);
    void documentsChangedFromCppEditorDocument(const QString &filePath);
    void documentsChangedIfNotCurrentDocument(Core::IDocument *document);
    void documentsChangedWithRevisionCheck(const ClangBackEnd::FileContainer &fileContainer);
    void documentsChangedWithRevisionCheck(Core::IDocument *document);
    void documentsClosed(const FileContainers &fileContainers);
    void documentVisibilityChanged();

    void unsavedFilesUpdated(Core::IDocument *document);
    void unsavedFilesUpdated(const QString &filePath,
                             const QByteArray &contents,
                             uint documentRevision);
    void unsavedFilesUpdated(const FileContainers &fileContainers);
    void unsavedFilesUpdatedFromCppEditorDocument(const QString &filePath);
    void unsavedFilesRemoved(const FileContainers &fileContainers);

    void requestCompletions(ClangCompletionAssistProcessor *assistProcessor,
                            const QString &filePath,
                            quint32 line,
                            quint32 column,
                            qint32 funcNameStartLine = -1,
                            qint32 funcNameStartColumn = -1);
    void cancelCompletions(TextEditor::IAssistProcessor *processor);
    void requestAnnotations(const ClangBackEnd::FileContainer &fileContainer);
    QFuture<CppTools::CursorInfo> requestReferences(
            const FileContainer &fileContainer,
            quint32 line,
            quint32 column,
            const LocalUseMap &localUses);
    QFuture<CppTools::CursorInfo> requestLocalReferences(
            const FileContainer &fileContainer,
            quint32 line,
            quint32 column);
    QFuture<CppTools::ToolTipInfo> requestToolTip(const FileContainer &fileContainer,
                                                  quint32 line,
                                                  quint32 column);
    QFuture<CppTools::SymbolInfo> requestFollowSymbol(const FileContainer &curFileContainer,
                                                      quint32 line,
                                                      quint32 column);

    void updateChangeContentStartPosition(const QString &filePath, int position);
    bool isNotWaitingForCompletion() const;

    void setBackendJobsPostponed(bool postponed);

private:
    void initializeBackend();
    void initializeBackendWithCurrentData();
    void restoreCppEditorDocuments();
    void resetCppEditorDocumentProcessors();
    void unsavedFilesUpdatedForUiHeaders();

    void setupDummySender();

    void onConnectedToBackend();
    void onEditorAboutToClose(Core::IEditor *editor);

    void logExecutableDoesNotExist();
    void logRestartedDueToUnexpectedFinish();
    void logStartTimeOut();
    void logError(const QString &text);

    void documentVisibilityChanged(const Utf8String &currentEditorFilePath,
                                   const Utf8StringVector &visibleEditorsFilePaths);

private:
    BackendReceiver m_receiver;
    ClangBackEnd::ClangCodeModelConnectionClient m_connection;
    QTimer m_backendStartTimeOut;
    QScopedPointer<ClangBackEnd::ClangCodeModelServerInterface> m_sender;
    int m_connectedCount = 0;
    bool m_postponeBackendJobs = false;
};

} // namespace Internal
} // namespace ClangCodeModel
