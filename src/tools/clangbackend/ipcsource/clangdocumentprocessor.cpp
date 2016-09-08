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

#include "clangjobs.h"

#include "clangdocument.h"

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
        , jobs(documents, unsavedFiles, projects, client)
    {}

public:
    Document document;
    Jobs jobs;
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

void DocumentProcessor::addJob(const JobRequest &jobRequest)
{
    d->jobs.add(jobRequest);
}

JobRequests DocumentProcessor::process()
{
    return d->jobs.process();
}

Document DocumentProcessor::document() const
{
    return d->document;
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
