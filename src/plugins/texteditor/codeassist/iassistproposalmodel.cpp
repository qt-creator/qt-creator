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

IAssistProposalModel::IAssistProposalModel()
{}

IAssistProposalModel::~IAssistProposalModel()
{}
