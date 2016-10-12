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

#include "clangjobrequest.h"

#include <functional>

namespace ClangBackEnd {

class ProjectParts;
class Documents;

class JobQueue
{
public:
    JobQueue(Documents &documents, ProjectParts &projects);

    bool add(const JobRequest &job);

    JobRequests processQueue();

    using IsJobRunningForTranslationUnitHandler = std::function<bool(const Utf8String &)>;
    void setIsJobRunningForTranslationUnitHandler(
            const IsJobRunningForTranslationUnitHandler &isJobRunningHandler);

    using IsJobRunningForJobRequestHandler = std::function<bool(const JobRequest &)>;
    void setIsJobRunningForJobRequestHandler(
            const IsJobRunningForJobRequestHandler &isJobRunningHandler);

public: // for tests
    JobRequests queue() const;
    int size() const;
    void prioritizeRequests();

private:
    bool isJobRunningForTranslationUnit(const Utf8String &translationUnitId);
    bool isJobRunningForJobRequest(const JobRequest &jobRequest);
    JobRequests takeJobRequestsToRunNow();
    void removeOutDatedRequests();
    bool isJobRequestOutDated(const JobRequest &jobRequest) const;

private:
    Documents &m_documents;
    ProjectParts &m_projectParts;

    IsJobRunningForTranslationUnitHandler m_isJobRunningForTranslationUnitHandler;
    IsJobRunningForJobRequestHandler m_isJobRunningForJobRequestHandler;

    JobRequests m_queue;
};

} // namespace ClangBackEnd
