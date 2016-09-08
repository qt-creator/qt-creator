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

#include "clangdocumentprocessor.h"
#include "clangjobs.h"

#include <utf8string.h>

#include <QMap>

namespace ClangBackEnd {

class Document;
class DocumentProcessor;

class DocumentId {
public:
    Utf8String filePath;
    Utf8String projectPartId;
};

class DocumentProcessors
{
public:
    DocumentProcessors(Documents &documents,
                       UnsavedFiles &unsavedFiles,
                       ProjectParts &projects,
                       ClangCodeModelClientInterface &client);

    DocumentProcessor create(const Document &document);
    DocumentProcessor processor(const Document &document);
    void remove(const Document &document);

    JobRequests process();

public: // for tests
    QList<DocumentProcessor> processors() const;
    QList<Jobs::RunningJob> runningJobs() const;
    int queueSize() const;

private:
    Documents &m_documents;
    UnsavedFiles &m_unsavedFiles;
    ProjectParts &m_projects;
    ClangCodeModelClientInterface &m_client;

    QMap<DocumentId, DocumentProcessor> m_processors;
};

} // namespace ClangBackEnd
