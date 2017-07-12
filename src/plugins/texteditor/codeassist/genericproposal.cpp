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

#include "genericproposal.h"
#include "genericproposalmodel.h"
#include "genericproposalwidget.h"

namespace TextEditor {

GenericProposal::GenericProposal(int cursorPos, GenericProposalModel *model)
    : IAssistProposal(cursorPos)
    , m_model(model)
{}

GenericProposal::GenericProposal(int cursorPos, const QList<AssistProposalItemInterface *> &items)
    : IAssistProposal(cursorPos)
    , m_model(new GenericProposalModel)
{
    m_model->loadContent(items);
}

GenericProposal::~GenericProposal()
{}

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

IAssistProposalModel *GenericProposal::model() const
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
