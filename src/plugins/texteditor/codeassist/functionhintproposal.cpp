// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "functionhintproposal.h"
#include "ifunctionhintproposalmodel.h"
#include "functionhintproposalwidget.h"

static const char functionHintId[] = "TextEditor.FunctionHintId";

using namespace TextEditor;

FunctionHintProposal::FunctionHintProposal(int cursorPos, FunctionHintProposalModelPtr model)
    : IAssistProposal(functionHintId, cursorPos)
    , m_model(model)
{}

FunctionHintProposal::~FunctionHintProposal() = default;

ProposalModelPtr FunctionHintProposal::model() const
{
    return m_model;
}

IAssistProposalWidget *FunctionHintProposal::createWidget() const
{
    return new FunctionHintProposalWidget;
}
