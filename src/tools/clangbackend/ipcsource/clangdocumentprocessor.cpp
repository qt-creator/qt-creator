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

#include "clangdocumentprocessor.h"

#include "clangdocuments.h"
#include "clangjobs.h"
#include "clangsupportivetranslationunitinitializer.h"

#include "clangdocument.h"
#include "clangtranslationunits.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace ClangBackEnd {

class DocumentProcessorData
{
public:
    DocumentProcessorData(const Document &document,
                          Documents &documents,
                          UnsavedFiles &unsavedFiles,
                          ProjectParts &projects,
                          ClangCodeModelClientInterface &client)
        : document(document)
        , documents(documents)
        , jobs(documents, unsavedFiles, projects, client)
        , supportiveTranslationUnitInitializer(document, jobs)
    {
        const auto isDocumentClosedChecker = [this](const Utf8String &filePath,
                                                    const Utf8String &projectPartId) {
            return !this->documents.hasDocument(filePath, projectPartId);
        };
        supportiveTranslationUnitInitializer.setIsDocumentClosedChecker(isDocumentClosedChecker);
    }

public:
    Document document;
    Documents &documents;
    Jobs jobs;

    SupportiveTranslationUnitInitializer supportiveTranslationUnitInitializer;
};

DocumentProcessor::DocumentProcessor(const Document &document,
                                     Documents &documents,
                                     UnsavedFiles &unsavedFiles,
                                     ProjectParts &projects,
                                     ClangCodeModelClientInterface &client)
    : d(std::make_shared<DocumentProcessorData>(document,
                                                documents,
                                                unsavedFiles,
                                                projects,
                                                client))
{
}

JobRequest DocumentProcessor::createJobRequest(
        JobRequest::Type type,
        PreferredTranslationUnit preferredTranslationUnit) const
{
    return d->jobs.createJobRequest(d->document, type, preferredTranslationUnit);
}

void DocumentProcessor::addJob(const JobRequest &jobRequest)
{
    d->jobs.add(jobRequest);
}

void DocumentProcessor::addJob(JobRequest::Type type, PreferredTranslationUnit preferredTranslationUnit)
{
    d->jobs.add(d->document, type, preferredTranslationUnit);
}

JobRequests DocumentProcessor::process()
{
    return d->jobs.process();
}

Document DocumentProcessor::document() const
{
    return d->document;
}

bool DocumentProcessor::hasSupportiveTranslationUnit() const
{
    return d->supportiveTranslationUnitInitializer.state()
        != SupportiveTranslationUnitInitializer::State::NotInitialized;
}

void DocumentProcessor::startInitializingSupportiveTranslationUnit()
{
    d->supportiveTranslationUnitInitializer.startInitializing();
}

bool DocumentProcessor::isSupportiveTranslationUnitInitialized() const
{
    return d->supportiveTranslationUnitInitializer.state()
        == SupportiveTranslationUnitInitializer::State::Initialized;
}

JobRequests &DocumentProcessor::queue()
{
    return d->jobs.queue();
}

QList<Jobs::RunningJob> DocumentProcessor::runningJobs() const
{
    return d->jobs.runningJobs();
}

int DocumentProcessor::queueSize() const
{
    return d->jobs.queue().size();
}

} // namespace ClangBackEnd
