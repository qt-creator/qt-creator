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
#include "projectpart.h"

namespace ClangBackEnd {

DocumentProcessors::DocumentProcessors(Documents &documents,
                                       UnsavedFiles &unsavedFiles,
                                       ProjectParts &projects,
                                       ClangCodeModelClientInterface &client)
    : m_documents(documents)
    , m_unsavedFiles(unsavedFiles)
    , m_projects(projects)
    , m_client(client)
{
}

static bool operator<(const DocumentId &lhs, const DocumentId &rhs)
{
    return lhs.filePath < rhs.filePath
        || (lhs.filePath == rhs.filePath && lhs.projectPartId < lhs.projectPartId);
}

DocumentProcessor DocumentProcessors::create(const Document &document)
{
    const DocumentId id{document.filePath(), document.projectPart().id()};
    if (m_processors.contains(id))
        throw DocumentProcessorAlreadyExists(document.filePath(), document.projectPart().id());

    const DocumentProcessor element(document, m_documents, m_unsavedFiles, m_projects, m_client);
    m_processors.insert(id, element);

    return element;
}

DocumentProcessor DocumentProcessors::processor(const Document &document)
{
    const DocumentId id{document.filePath(), document.projectPart().id()};

    const auto it = m_processors.find(id);
    if (it == m_processors.end())
        throw DocumentProcessorDoesNotExist(document.filePath(), document.projectPart().id());

    return *it;
}

QList<DocumentProcessor> DocumentProcessors::processors() const
{
    return m_processors.values();
}

void DocumentProcessors::remove(const Document &document)
{
    const DocumentId id{document.filePath(), document.projectPart().id()};

    const int itemsRemoved = m_processors.remove(id);
    if (itemsRemoved != 1)
        throw DocumentProcessorDoesNotExist(document.filePath(), document.projectPart().id());
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
