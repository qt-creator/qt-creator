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

#include "quickfixassistprocessor.h"
#include "quickfixassistprovider.h"
#include "iassistinterface.h"
#include "basicproposalitemlistmodel.h"
#include "basicproposalitem.h"
#include "genericproposal.h"

// @TODO: Move...
#include <texteditor/quickfix.h>

#include <QMap>
#include <QDebug>

using namespace TextEditor;

QuickFixAssistProcessor::QuickFixAssistProcessor()
{}

QuickFixAssistProcessor::~QuickFixAssistProcessor()
{}

IAssistProposal *QuickFixAssistProcessor::perform(const IAssistInterface *interface)
{
    if (!interface)
        return 0;

    QSharedPointer<const IAssistInterface> assistInterface(interface);

    QuickFixOperations quickFixes;

    const QuickFixAssistProvider *quickFixProvider =
            static_cast<const QuickFixAssistProvider *>(provider());
    foreach (QuickFixFactory *factory, quickFixProvider->quickFixFactories())
        factory->matchingOperations(assistInterface, quickFixes);

    if (!quickFixes.isEmpty()) {
        QList<BasicProposalItem *> items;
        foreach (const QuickFixOperation::Ptr &op, quickFixes) {
            QVariant v;
            v.setValue(op);
            BasicProposalItem *item = new BasicProposalItem;
            item->setText(op->description());
            item->setData(v);
            item->setOrder(op->priority());
            items.append(item);
        }
        return new GenericProposal(interface->position(), new BasicProposalItemListModel(items));
    }

    return 0;
}
