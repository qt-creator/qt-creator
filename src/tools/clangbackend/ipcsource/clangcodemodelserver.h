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

#include "clangcodemodelserverinterface.h"

#include "projectpart.h"
#include "projects.h"
#include "clangdocument.h"
#include "clangdocuments.h"
#include "unsavedfiles.h"
#include "clangjobs.h"

#include <utf8string.h>

#include <QScopedPointer>
#include <QTimer>

namespace ClangBackEnd {

class ClangCodeModelServer : public ClangCodeModelServerInterface
{
public:
    ClangCodeModelServer();

    void end() override;
    void registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message) override;
    void updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message) override;
    void unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message) override;
    void registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message) override;
    void unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message) override;
    void registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message) override;
    void unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message) override;
    void completeCode(const CompleteCodeMessage &message) override;
    void updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message) override;
    void requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message) override;

public /*for tests*/:
    const Documents &documentsForTestOnly() const;
    const Jobs &jobsForTestOnly();
    bool isTimerRunningForTestOnly() const;
    void setUpdateDocumentAnnotationsTimeOutInMsForTestsOnly(int value);

private:
    Jobs &jobs();

    void startDocumentAnnotationsTimerIfFileIsNotATranslationUnit(const Utf8String &filePath);
    void addJobRequestsForDirtyAndVisibleDocuments();
    void processJobsForDirtyAndVisibleDocuments();
    void processInitialJobsForDocuments(const std::vector<Document> &documents);

    JobRequest createJobRequest(const Document &document, JobRequest::Type type) const;

private:
    ProjectParts projects;
    UnsavedFiles unsavedFiles;
    Documents documents;
    QScopedPointer<Jobs> jobs_;

    QTimer updateDocumentAnnotationsTimer;
    int updateDocumentAnnotationsTimeOutInMs;
};

} // namespace ClangBackEnd
