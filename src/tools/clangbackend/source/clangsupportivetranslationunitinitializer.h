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

#include "clangdocument.h"
#include "clangjobs.h"

#include <functional>

#pragma once

namespace ClangBackEnd {

class SupportiveTranslationUnitInitializer
{
public:
    using IsDocumentClosedChecker = std::function<bool(const Utf8String &, const Utf8String &)>;

    enum class State {
        NotInitialized,
        WaitingForParseJob,
        WaitingForReparseJob,
        Initialized,
        Aborted
    };

public:
    SupportiveTranslationUnitInitializer(const Document &document, Jobs &jobs);

    void setIsDocumentClosedChecker(const IsDocumentClosedChecker &isDocumentClosedChecker);

    State state() const;
    void startInitializing();

public: // for tests
    void setState(const State &state);
    void checkIfParseJobFinished(const Jobs::RunningJob &job);
    void checkIfReparseJobFinished(const Jobs::RunningJob &job);

private:
    bool abortIfDocumentIsClosed();
    void addJob(JobRequest::Type jobRequestType);

private:
    Document m_document;
    Jobs &m_jobs;

    State m_state = State::NotInitialized;
    IsDocumentClosedChecker m_isDocumentClosedChecker;
};

} // namespace ClangBackEnd
