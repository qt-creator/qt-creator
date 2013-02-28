/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "runner.h"
#include "iassistprocessor.h"
#include "iassistproposal.h"
#include "iassistinterface.h"
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

void ProcessorRunner::setReason(AssistReason reason)
{
    m_reason = reason;
}

AssistReason ProcessorRunner::reason() const
{
    return m_reason;
}

void ProcessorRunner::setDiscardProposal(bool discard)
{
    m_discardProposal = discard;
}

void ProcessorRunner::setAssistInterface(IAssistInterface *interface)
{
    m_interface = interface;
}
