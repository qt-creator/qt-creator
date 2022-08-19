// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "runner.h"
#include "iassistprocessor.h"
#include "iassistproposal.h"
#include "assistinterface.h"
#include "iassistproposalmodel.h"

using namespace TextEditor;
using namespace Internal;

ProcessorRunner::ProcessorRunner() = default;

ProcessorRunner::~ProcessorRunner()
{
    delete m_processor;
    if (m_discardProposal && m_proposal)
        delete m_proposal;
}

void ProcessorRunner::setProcessor(IAssistProcessor *computer)
{
    m_processor = computer;
}

void ProcessorRunner::run()
{
    m_interface->recreateTextDocument();
    m_proposal = m_processor->perform(m_interface);
}

IAssistProposal *ProcessorRunner::proposal() const
{
    return m_proposal;
}

void ProcessorRunner::setDiscardProposal(bool discard)
{
    m_discardProposal = discard;
}

void ProcessorRunner::setAssistInterface(AssistInterface *interface)
{
    m_interface = interface;
}
