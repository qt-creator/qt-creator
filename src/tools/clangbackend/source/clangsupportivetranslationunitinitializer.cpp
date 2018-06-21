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
#include "projectpart.h"

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
    if (!checkStateAndDocument(State::NotInitialized))
        return;

    m_document.translationUnits().createAndAppend();

    m_jobs.setJobFinishedCallback([this](const Jobs::RunningJob &runningJob) {
        checkIfParseJobFinished(runningJob);
    });
    addJob(JobRequest::Type::ParseSupportiveTranslationUnit);
    m_jobs.process();

    m_state = State::WaitingForParseJob;
}

void SupportiveTranslationUnitInitializer::abort()
{
    m_jobs.setJobFinishedCallback(Jobs::JobFinishedCallback());
    m_state = State::Aborted;
}

void SupportiveTranslationUnitInitializer::checkIfParseJobFinished(const Jobs::RunningJob &job)
{
    if (!checkStateAndDocument(State::WaitingForParseJob))
        return;

    if (job.jobRequest.type == JobRequest::Type::ParseSupportiveTranslationUnit) {
        if (m_document.translationUnits().areAllTranslationUnitsParsed()) {
            m_jobs.setJobFinishedCallback(nullptr);
            m_state = State::Initialized;
        } else {
            // The supportive translation unit was parsed, but the document
            // revision changed in the meanwhile, so try again.
            addJob(JobRequest::Type::ParseSupportiveTranslationUnit);
        }
    }
}

bool SupportiveTranslationUnitInitializer::checkStateAndDocument(State currentExpectedState)
{
    if (m_state != currentExpectedState) {
        m_state = State::Aborted;
        return false;
    }

    QTC_CHECK(m_isDocumentClosedChecker);
    if (m_isDocumentClosedChecker(m_document.filePath(), m_document.projectPart().id())) {
        m_state = State::Aborted;
        return false;
    }

    return true;
}

void SupportiveTranslationUnitInitializer::addJob(JobRequest::Type jobRequestType)
{
    const JobRequest jobRequest = m_jobs.createJobRequest(
                m_document, jobRequestType, PreferredTranslationUnit::LastUninitialized);

    m_jobs.add(jobRequest);
}

void SupportiveTranslationUnitInitializer::setState(const State &state)
{
    m_state = state;
}

} // namespace ClangBackEnd
