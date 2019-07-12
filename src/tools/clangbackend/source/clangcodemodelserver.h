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

#include "clangdocument.h"
#include "clangdocuments.h"
#include "clangdocumentprocessors.h"
#include "clangjobrequest.h"
#include "unsavedfiles.h"

#include <clangcodemodelserverinterface.h>
#include <ipcclientprovider.h>
#include <utf8string.h>

#include <QScopedPointer>
#include <QTimer>

namespace ClangBackEnd {

struct DocumentResetInfo {
    Document documentToRemove;
    FileContainer fileContainer;
};
using DocumentResetInfos = QVector<DocumentResetInfo>;

class ClangCodeModelServer : public ClangCodeModelServerInterface,
                             public IpcClientProvider<ClangCodeModelClientInterface>
{
public:
    ClangCodeModelServer();

    void end() override;

    void documentsOpened(const DocumentsOpenedMessage &message) override;
    void documentsChanged(const DocumentsChangedMessage &message) override;
    void documentsClosed(const DocumentsClosedMessage &message) override;
    void documentVisibilityChanged(const DocumentVisibilityChangedMessage &message) override;

    void unsavedFilesUpdated(const UnsavedFilesUpdatedMessage &message) override;
    void unsavedFilesRemoved(const UnsavedFilesRemovedMessage &message) override;

    void requestCompletions(const RequestCompletionsMessage &message) override;
    void requestAnnotations(const RequestAnnotationsMessage &message) override;
    void requestReferences(const RequestReferencesMessage &message) override;
    void requestFollowSymbol(const RequestFollowSymbolMessage &message) override;
    void requestToolTip(const RequestToolTipMessage &message) override;

public: // for tests
    const Documents &documentsForTestOnly() const;
    QList<Jobs::RunningJob> runningJobsForTestsOnly();
    int queueSizeForTestsOnly();
    bool isTimerRunningForTestOnly() const;
    void setUpdateAnnotationsTimeOutInMsForTestsOnly(int value);
    void setUpdateVisibleButNotCurrentDocumentsTimeOutInMsForTestsOnly(int value);
    DocumentProcessors &documentProcessors();

private:
    void processJobsForVisibleDocuments();
    void processJobsForCurrentDocument();
    void processTimerForVisibleButNotCurrentDocuments();
    void processSuspendResumeJobs(const std::vector<Document> &documents);

    bool onJobFinished(const Jobs::RunningJob &jobRecord, IAsyncJob *job);

    void categorizeFileContainers(const QVector<FileContainer> &fileContainers,
                                  QVector<FileContainer> &toCreate,
                                  DocumentResetInfos &toReset) const;
    std::vector<Document> resetDocuments(const DocumentResetInfos &infos);
    int resetDocumentsWithUnresolvedIncludes(const std::vector<Document> &documents);

    void addAndRunUpdateJobs(std::vector<Document> documents);

private:
    UnsavedFiles unsavedFiles;
    Documents documents;

    QScopedPointer<DocumentProcessors> documentProcessors_; // Delayed initialization

    QTimer updateAnnotationsTimer;
    int updateAnnotationsTimeOutInMs = 1500;

    QTimer updateVisibleButNotCurrentDocumentsTimer;
    int updateVisibleButNotCurrentDocumentsTimeOutInMs = 2000;
};

} // namespace ClangBackEnd
