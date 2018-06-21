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

#include "clangiasyncjob.h"

Q_LOGGING_CATEGORY(jobsLog, "qtc.clangbackend.jobs");

namespace ClangBackEnd {

IAsyncJob::IAsyncJob()
    : m_context(JobContext())
{
}

IAsyncJob::~IAsyncJob()
{
}

JobContext IAsyncJob::context() const
{
    return m_context;
}

void IAsyncJob::setContext(const JobContext &context)
{
    m_context = context;
}

IAsyncJob::FinishedHandler IAsyncJob::finishedHandler() const
{
    return m_finishedHandler;
}

void IAsyncJob::setFinishedHandler(const IAsyncJob::FinishedHandler &finishedHandler)
{
    m_finishedHandler = finishedHandler;
}

bool IAsyncJob::isFinished() const
{
    return m_isFinished;
}

void IAsyncJob::setIsFinished(bool isFinished)
{
    m_isFinished = isFinished;
}

} // namespace ClangBackEnd
