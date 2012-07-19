/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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

    QList<QuickFixOperation::Ptr> quickFixes;

    const QuickFixAssistProvider *quickFixProvider =
            static_cast<const QuickFixAssistProvider *>(provider());
    foreach (QuickFixFactory *factory, quickFixProvider->quickFixFactories())
        quickFixes += factory->matchingOperations(assistInterface);

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
