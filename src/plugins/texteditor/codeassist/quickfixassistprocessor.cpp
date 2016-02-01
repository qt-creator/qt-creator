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

#include "quickfixassistprocessor.h"
#include "quickfixassistprovider.h"
#include "assistinterface.h"
#include "genericproposalmodel.h"
#include "assistproposalitem.h"
#include "genericproposal.h"

// @TODO: Move...
#include <texteditor/quickfix.h>

namespace TextEditor {

QuickFixAssistProcessor::QuickFixAssistProcessor(const QuickFixAssistProvider *provider)
    : m_provider(provider)
{}

QuickFixAssistProcessor::~QuickFixAssistProcessor()
{}

IAssistProposal *QuickFixAssistProcessor::perform(const AssistInterface *interface)
{
    if (!interface)
        return 0;

    QSharedPointer<const AssistInterface> assistInterface(interface);

    QuickFixOperations quickFixes;

    foreach (QuickFixFactory *factory, m_provider->quickFixFactories())
        factory->matchingOperations(assistInterface, quickFixes);

    if (!quickFixes.isEmpty()) {
        QList<AssistProposalItemInterface *> items;
        foreach (const QuickFixOperation::Ptr &op, quickFixes) {
            QVariant v;
            v.setValue(op);
            AssistProposalItem *item = new AssistProposalItem;
            item->setText(op->description());
            item->setData(v);
            item->setOrder(op->priority());
            items.append(item);
        }

        return new GenericProposal(interface->position(), items);
    }

    return 0;
}

} // namespace TextEdit
