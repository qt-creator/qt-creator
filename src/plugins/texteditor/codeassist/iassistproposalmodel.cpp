// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iassistproposalmodel.h"

using namespace TextEditor;

/*!
    \class TextEditor::IAssistProposalModel
    \brief The IAssistProposalModel class acts as an interface for representing proposals.
    \ingroup CodeAssist

    Known implenters of this interface are IFunctionHintProposalModel and GenericProposalModel.
    The former is recommeded to be used when assisting function calls constructs (overloads
    and parameters) while the latter is quite generic so that it could be used to propose
    snippets, refactoring operations (quickfixes), and contextual content (the member of class
    or a string existent in the document, for example).

    This is class is part of the CodeAssist API.
*/

IAssistProposalModel::IAssistProposalModel() = default;

IAssistProposalModel::~IAssistProposalModel() = default;
