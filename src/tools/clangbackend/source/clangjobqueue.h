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

class Documents;

class JobQueue
{
public:
    JobQueue(Documents &documents, const Utf8String &logTag = Utf8String());

    bool add(const JobRequest &job);

    JobRequests processQueue();

    using IsJobRunningForTranslationUnitHandler = std::function<bool(const Utf8String &)>;
    void setIsJobRunningForTranslationUnitHandler(
            const IsJobRunningForTranslationUnitHandler &isJobRunningHandler);

    using IsJobRunningForJobRequestHandler = std::function<bool(const JobRequest &)>;
    void setIsJobRunningForJobRequestHandler(
            const IsJobRunningForJobRequestHandler &isJobRunningHandler);

    using CancelJobRequest = std::function<void(const JobRequest &)>;
    void setCancelJobRequest(const CancelJobRequest &cancelJobRequest);

public: // for tests
    JobRequests &queue();
    const JobRequests &queue() const;
    int size() const;
    void prioritizeRequests();

private:
    bool areRunConditionsMet(const JobRequest &request, const Document &document) const;
    void cancelJobRequest(const JobRequest &jobRequest);
    bool isJobRunningForTranslationUnit(const Utf8String &translationUnitId);
    bool isJobRunningForJobRequest(const JobRequest &jobRequest);
    JobRequests takeJobRequestsToRunNow();
    void removeExpiredRequests();
    bool isJobRequestAddable(const JobRequest &jobRequest, QString &notAddableReason);
    bool isJobRequestExpired(const JobRequest &jobRequest, QString &expirationReason);

private:
    Documents &m_documents;
    Utf8String m_logTag;

    IsJobRunningForTranslationUnitHandler m_isJobRunningForTranslationUnitHandler;
    IsJobRunningForJobRequestHandler m_isJobRunningForJobRequestHandler;
    CancelJobRequest m_cancelJobRequest;

    JobRequests m_queue;
};

} // namespace ClangBackEnd
