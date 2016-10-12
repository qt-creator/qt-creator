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

#include "clangjobcontext.h"

#include <QFuture>
#include <QLoggingCategory>

#include <functional>

Q_DECLARE_LOGGING_CATEGORY(jobsLog);

namespace ClangBackEnd {

class IAsyncJob
{
public:
    static IAsyncJob *create(JobRequest::Type type);

    struct AsyncPrepareResult {
        operator bool() const { return !translationUnitId.isEmpty(); }
        Utf8String translationUnitId;
    };

public:
    IAsyncJob();
    virtual ~IAsyncJob();

    JobContext context() const;
    void setContext(const JobContext &context);

    using FinishedHandler = std::function<void(IAsyncJob *job)>;
    FinishedHandler finishedHandler() const;
    void setFinishedHandler(const FinishedHandler &finishedHandler);

    virtual AsyncPrepareResult prepareAsyncRun() = 0;
    virtual QFuture<void> runAsync() = 0;
    virtual void finalizeAsyncRun() = 0;

    virtual void preventFinalization() = 0;

public: // for tests
    bool isFinished() const;
    void setIsFinished(bool isFinished);

private:
    bool m_isFinished = false;
    FinishedHandler m_finishedHandler;
    JobContext m_context;
};

} // namespace ClangBackEnd
