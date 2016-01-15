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

#include "iassistprocessor.h"

using namespace TextEditor;

/*!
    \class TextEditor::IAssistProcessor
    \brief The IAssistProcessor class acts as an interface that actually computes an assist
    proposal.
    \ingroup CodeAssist

    \sa IAssistProposal, IAssistProvider
*/

IAssistProcessor::IAssistProcessor()
{}

IAssistProcessor::~IAssistProcessor()
{}

void IAssistProcessor::setAsyncProposalAvailable(IAssistProposal *proposal)
{
    if (m_asyncCompletionsAvailableHandler)
        m_asyncCompletionsAvailableHandler(proposal);
}

void IAssistProcessor::setAsyncCompletionAvailableHandler(
        const IAssistProcessor::AsyncCompletionsAvailableHandler &finalizer)
{
    m_asyncCompletionsAvailableHandler = finalizer;
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
