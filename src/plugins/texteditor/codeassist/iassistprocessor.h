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

#include <texteditor/texteditor_global.h>

#include <functional>

namespace TextEditor {

class IAssistProvider;
class AssistInterface;
class IAssistProposal;

class TEXTEDITOR_EXPORT IAssistProcessor
{
public:
    IAssistProcessor();
    virtual ~IAssistProcessor();

    virtual IAssistProposal *immediateProposal(const AssistInterface *) { return 0; }
    virtual IAssistProposal *perform(const AssistInterface *interface) = 0;

    void setAsyncProposalAvailable(IAssistProposal *proposal);

    // Internal, used by CodeAssist
    using AsyncCompletionsAvailableHandler = std::function<void (IAssistProposal *proposal)>;
    void setAsyncCompletionAvailableHandler(const AsyncCompletionsAvailableHandler &finalizer);

    bool performWasApplicable() { return m_performWasApplicable; }
    void setPerformWasApplicable(bool applicable) { m_performWasApplicable = applicable; }

private:
    AsyncCompletionsAvailableHandler m_asyncCompletionsAvailableHandler;
    bool m_performWasApplicable = true;
};

} // TextEditor
