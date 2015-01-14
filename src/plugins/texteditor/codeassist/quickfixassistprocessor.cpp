/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        QList<AssistProposalItem *> items;
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
