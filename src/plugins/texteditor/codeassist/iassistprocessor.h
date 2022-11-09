// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include <functional>

namespace TextEditor {

class AssistInterface;
class IAssistProposal;

class TEXTEDITOR_EXPORT IAssistProcessor
{
public:
    IAssistProcessor();
    virtual ~IAssistProcessor();

    virtual IAssistProposal *immediateProposal(const AssistInterface *) { return nullptr; }
    virtual IAssistProposal *perform(const AssistInterface *interface) = 0; // takes ownership

    void setAsyncProposalAvailable(IAssistProposal *proposal);

    // Internal, used by CodeAssist
    using AsyncCompletionsAvailableHandler
        = std::function<void (IAssistProposal *proposal)>;
    void setAsyncCompletionAvailableHandler(const AsyncCompletionsAvailableHandler &handler);

    virtual bool running() { return false; }
    virtual bool needsRestart() const { return false; }
    virtual void cancel() {}

private:
    AsyncCompletionsAvailableHandler m_asyncCompletionsAvailableHandler;
};

} // TextEditor
