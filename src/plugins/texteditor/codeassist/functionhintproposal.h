// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iassistproposal.h"
#include "ifunctionhintproposalmodel.h"

namespace TextEditor {

class TEXTEDITOR_EXPORT FunctionHintProposal : public IAssistProposal
{
public:
    FunctionHintProposal(int cursorPos, FunctionHintProposalModelPtr model);
    ~FunctionHintProposal() override;

    ProposalModelPtr model() const override;
    IAssistProposalWidget *createWidget() const override;

private:
    FunctionHintProposalModelPtr m_model;
};

} // TextEditor
