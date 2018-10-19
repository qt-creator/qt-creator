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

#include "clangdocumentprocessors.h"
#include "clangdocument.h"
#include "clangexceptions.h"

#include <utils/algorithm.h>

namespace ClangBackEnd {

DocumentProcessors::DocumentProcessors(Documents &documents,
                                       UnsavedFiles &unsavedFiles,
                                       ClangCodeModelClientInterface &client)
    : m_documents(documents)
    , m_unsavedFiles(unsavedFiles)
    , m_client(client)
{
}

DocumentProcessor DocumentProcessors::create(const Document &document)
{
    const Utf8String filePath{document.filePath()};
    if (m_processors.contains(filePath))
        throw DocumentProcessorAlreadyExists(document.filePath());

    const DocumentProcessor element(document, m_documents, m_unsavedFiles, m_client);
    m_processors.insert(filePath, element);

    return element;
}

DocumentProcessor DocumentProcessors::processor(const Document &document)
{
    const Utf8String filePath = document.filePath();
    const auto it = m_processors.find(filePath);
    if (it == m_processors.end())
        throw DocumentProcessorDoesNotExist(filePath);

    return *it;
}

QList<DocumentProcessor> DocumentProcessors::processors() const
{
    return m_processors.values();
}

void DocumentProcessors::remove(const Document &document)
{
    const int itemsRemoved = m_processors.remove(document.filePath());
    if (itemsRemoved != 1)
        throw DocumentProcessorDoesNotExist(document.filePath());
}

void DocumentProcessors::reset(const Document &oldDocument, const Document &newDocument)
{
    // Wait until the currently running jobs finish and remember the not yet
    // processed job requests for the new processor...
    const JobRequests jobsStillInQueue = processor(oldDocument).stop();
    // ...but do not take over irrelevant ones.
    const JobRequests jobsForNewProcessor = Utils::filtered(jobsStillInQueue,
                                                            [](const JobRequest &job) {
        return job.isTakeOverable();
    });
    const Jobs::JobFinishedCallback jobFinishedCallback
        = processor(oldDocument).jobs().jobFinishedCallback();

    // Remove current processor
    remove(oldDocument);

    // Create new processor and take over not yet processed jobs.
    DocumentProcessor newProcessor = create(newDocument);
    for (const JobRequest &job : jobsForNewProcessor)
        newProcessor.addJob(job);
    newProcessor.jobs().setJobFinishedCallback(jobFinishedCallback);
}

JobRequests DocumentProcessors::process()
{
    JobRequests jobsStarted;
    for (auto &processor : m_processors)
        jobsStarted += processor.process();

    return jobsStarted;
}

QList<Jobs::RunningJob> DocumentProcessors::runningJobs() const
{
    QList<Jobs::RunningJob> jobs;
    for (auto &processor : m_processors)
        jobs += processor.runningJobs();

    return jobs;
}

int DocumentProcessors::queueSize() const
{
    int total = 0;
    for (auto &processor : m_processors)
        total += processor.queueSize();

    return total;
}

} // namespace ClangBackEnd
