// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assistinterface.h"
#include "assistproposalitem.h"
#include "genericproposal.h"
#include "genericproposalmodel.h"
#include "genericproposalwidget.h"
#include "../texteditorconstants.h"

namespace TextEditor {

GenericProposal::GenericProposal(int cursorPos, GenericProposalModelPtr model)
    : IAssistProposal(Constants::GENERIC_PROPOSAL_ID, cursorPos)
    , m_model(model)
{}

GenericProposal::GenericProposal(int cursorPos, const QList<AssistProposalItemInterface *> &items)
    : IAssistProposal(Constants::GENERIC_PROPOSAL_ID, cursorPos)
    , m_model(new GenericProposalModel)
{
    m_model->loadContent(items);
}

GenericProposal::~GenericProposal() = default;

GenericProposal *GenericProposal::createProposal(const AssistInterface *interface,
                                                 const QuickFixOperations &quickFixes)
{
    if (quickFixes.isEmpty())
        return nullptr;

    QList<AssistProposalItemInterface *> items;
    for (const QuickFixOperation::Ptr &op : quickFixes) {
        QVariant v;
        v.setValue(op);
        auto item = new AssistProposalItem;
        item->setText(op->description());
        item->setData(v);
        item->setOrder(op->priority());
        items.append(item);
    }

    return new GenericProposal(interface->position(), items);
}

bool GenericProposal::hasItemsToPropose(const QString &prefix, AssistReason reason) const
{
    if (!prefix.isEmpty()) {
        if (m_model->containsDuplicates())
            m_model->removeDuplicates();
        m_model->filter(prefix);
        m_model->setPrefilterPrefix(prefix);
    }

    return m_model->hasItemsToPropose(prefix, reason);
}

ProposalModelPtr GenericProposal::model() const
{
    return m_model;
}

IAssistProposalWidget *GenericProposal::createWidget() const
{
    return new GenericProposalWidget;
}

void GenericProposal::moveBasePosition(int length)
{
    m_basePosition += length;
}

} // namespace TextEditor
