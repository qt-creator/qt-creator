// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iassistprocessor.h"

#include "assistinterface.h"

#include <utils/qtcassert.h>

using namespace TextEditor;

/*!
    \class TextEditor::IAssistProcessor
    \brief The IAssistProcessor class acts as an interface that actually computes an assist
    proposal.
    \ingroup CodeAssist

    \sa IAssistProposal, IAssistProvider
*/

IAssistProcessor::IAssistProcessor() = default;

IAssistProcessor::~IAssistProcessor() = default;

IAssistProposal *IAssistProcessor::start(std::unique_ptr<AssistInterface> &&interface)
{
    QTC_ASSERT(!running(), return nullptr);
    m_interface = std::move(interface);
    QTC_ASSERT(m_interface, return nullptr);
    return perform();
}

void IAssistProcessor::setAsyncProposalAvailable(IAssistProposal *proposal)
{
    if (m_asyncCompletionsAvailableHandler)
        m_asyncCompletionsAvailableHandler(proposal);
}

void IAssistProcessor::setAsyncCompletionAvailableHandler(
        const IAssistProcessor::AsyncCompletionsAvailableHandler &handler)
{
    m_asyncCompletionsAvailableHandler = handler;
}

bool IAssistProcessor::running() { return false; }

bool IAssistProcessor::needsRestart() const { return false; }

void IAssistProcessor::cancel() {}

AssistInterface *IAssistProcessor::interface() { return m_interface.get(); }
const AssistInterface *IAssistProcessor::interface() const { return m_interface.get(); }

#ifdef WITH_TESTS
void IAssistProcessor::setupAssistInterface(std::unique_ptr<AssistInterface> &&interface)
{
    m_interface = std::move(interface);
}
#endif

/*!
    \fn IAssistProposal *TextEditor::IAssistProcessor::start()

    Computes a proposal and returns it. Access to the document is made through the \a interface.

    The processor takes ownership of the interface. Also, one should be careful in the case of
    sharing data across asynchronous processors since there might be more than one instance of
    them computing a proposal at a particular time.*/
