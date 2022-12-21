// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iassistproposal.h"
#include "genericproposalmodel.h"

#include <texteditor/quickfix.h>

namespace TextEditor {

class AssistProposalItemInterface;

class TEXTEDITOR_EXPORT GenericProposal : public IAssistProposal
{
public:
    GenericProposal(int cursorPos, GenericProposalModelPtr model);
    GenericProposal(int cursorPos, const QList<AssistProposalItemInterface *> &items);

    ~GenericProposal() override;

    static GenericProposal *createProposal(const AssistInterface *interface,
                                           const QuickFixOperations &quickFixes);

    bool hasItemsToPropose(const QString &prefix, AssistReason reason) const override;
    ProposalModelPtr model() const override;
    IAssistProposalWidget *createWidget() const override;

protected:
    void moveBasePosition(int length);

private:
    GenericProposalModelPtr m_model;
};

} // TextEditor
