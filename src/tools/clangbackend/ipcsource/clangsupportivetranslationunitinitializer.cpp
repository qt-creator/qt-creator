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

#include "clangsupportivetranslationunitinitializer.h"

#include "clangjobs.h"
#include "clangtranslationunits.h"

#include <utils/qtcassert.h>

namespace ClangBackEnd {

    // TODO: Check translation unit id?

SupportiveTranslationUnitInitializer::SupportiveTranslationUnitInitializer(
        const Document &document,
        Jobs &jobs)
    : m_document(document)
    , m_jobs(jobs)
{
}

void SupportiveTranslationUnitInitializer::setJobRequestCreator(const JobRequestCreator &creator)
{
    m_jobRequestCreator = creator;
}

void SupportiveTranslationUnitInitializer::setIsDocumentClosedChecker(
        const IsDocumentClosedChecker &isDocumentClosedChecker)
{
    m_isDocumentClosedChecker = isDocumentClosedChecker;
}

SupportiveTranslationUnitInitializer::State SupportiveTranslationUnitInitializer::state() const
{
    return m_state;
}

void SupportiveTranslationUnitInitializer::startInitializing()
{
    QTC_CHECK(m_state == State::NotInitialized);
    if (abortIfDocumentIsClosed())
        return;

    m_document.translationUnits().createAndAppend();

    m_jobs.setJobFinishedCallback([this](const Jobs::RunningJob &runningJob) {
        checkIfParseJobFinished(runningJob);
    });
    addJob(JobRequest::Type::ParseSupportiveTranslationUnit);
    m_jobs.process();

    m_state = State::WaitingForParseJob;
}

void SupportiveTranslationUnitInitializer::checkIfParseJobFinished(const Jobs::RunningJob &job)
{
    QTC_CHECK(m_state == State::WaitingForParseJob);
    if (abortIfDocumentIsClosed())
        return;

    if (job.jobRequest.type == JobRequest::Type::ParseSupportiveTranslationUnit) {
        m_jobs.setJobFinishedCallback([this](const Jobs::RunningJob &runningJob) {
            checkIfReparseJobFinished(runningJob);
        });

        addJob(JobRequest::Type::ReparseSupportiveTranslationUnit);

        m_state = State::WaitingForReparseJob;
    }
}

void SupportiveTranslationUnitInitializer::checkIfReparseJobFinished(const Jobs::RunningJob &job)
{
    QTC_CHECK(m_state == State::WaitingForReparseJob);
    if (abortIfDocumentIsClosed())
        return;

    if (job.jobRequest.type == JobRequest::Type::ReparseSupportiveTranslationUnit) {
        if (m_document.translationUnits().areAllTranslationUnitsParsed()) {
            m_jobs.setJobFinishedCallback(nullptr);
            m_state = State::Initialized;
        } else {
            // The supportive translation unit was reparsed, but the document
            // revision changed in the meanwhile, so try again.
            addJob(JobRequest::Type::ReparseSupportiveTranslationUnit);
        }
    }
}

bool SupportiveTranslationUnitInitializer::abortIfDocumentIsClosed()
{
    QTC_CHECK(m_isDocumentClosedChecker);

    if (m_isDocumentClosedChecker(m_document.filePath(), m_document.projectPartId())) {
        m_state = State::Aborted;
        return true;
    }

    return false;
}

void SupportiveTranslationUnitInitializer::addJob(JobRequest::Type jobRequestType)
{
    QTC_CHECK(m_jobRequestCreator);

    const JobRequest jobRequest = m_jobRequestCreator(m_document,
                                                      jobRequestType,
                                                      PreferredTranslationUnit::LastUninitialized);

    m_jobs.add(jobRequest);
}

void SupportiveTranslationUnitInitializer::setState(const State &state)
{
    m_state = state;
}

} // namespace ClangBackEnd
