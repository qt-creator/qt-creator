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

#include "runner.h"
#include "iassistprocessor.h"
#include "iassistproposal.h"
#include "assistinterface.h"
#include "iassistproposalmodel.h"

using namespace TextEditor;
using namespace Internal;

ProcessorRunner::ProcessorRunner()
    : m_processor(0)
    , m_interface(0)
    , m_discardProposal(false)
    , m_proposal(0)
{}

ProcessorRunner::~ProcessorRunner()
{
    delete m_processor;
    if (m_discardProposal && m_proposal) {
        // Proposal doesn't own the model, so we need to delete both.
        delete m_proposal->model();
        delete m_proposal;
    }
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
