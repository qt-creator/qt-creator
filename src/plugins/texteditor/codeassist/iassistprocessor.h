// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include <functional>
#include <memory>

namespace TextEditor {

class AssistInterface;
class IAssistProposal;

class TEXTEDITOR_EXPORT IAssistProcessor
{
public:
    IAssistProcessor();
    virtual ~IAssistProcessor();

    IAssistProposal *start(std::unique_ptr<AssistInterface> &&interface);

    // Internal, used by CodeAssist
    using AsyncCompletionsAvailableHandler
        = std::function<void (IAssistProposal *proposal)>;
    void setAsyncCompletionAvailableHandler(const AsyncCompletionsAvailableHandler &handler);
    void setAsyncProposalAvailable(IAssistProposal *proposal);

    virtual bool running();
    virtual bool needsRestart() const;
    virtual void cancel();

#ifdef WITH_TESTS
    void setupAssistInterface(std::unique_ptr<AssistInterface> &&interface);
#endif

protected:
    virtual IAssistProposal *perform() = 0;
    AssistInterface *interface();
    const AssistInterface *interface() const;

private:
    AsyncCompletionsAvailableHandler m_asyncCompletionsAvailableHandler;
    std::unique_ptr<AssistInterface> m_interface;
};

} // TextEditor
