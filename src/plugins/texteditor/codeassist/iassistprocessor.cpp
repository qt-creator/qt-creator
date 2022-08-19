// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "iassistprocessor.h"

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

/*!
    \fn IAssistProposal *TextEditor::IAssistProcessor::perform(const AssistInterface *interface)

    Computes a proposal and returns it. Access to the document is made through the \a interface.
    If this is an asynchronous processor the \a interface will be detached.

    The processor takes ownership of the interface. Also, one should be careful in the case of
    sharing data across asynchronous processors since there might be more than one instance of
    them computing a proposal at a particular time.

    \sa AssistInterface::detach()
*/
