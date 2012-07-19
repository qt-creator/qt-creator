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

#include "iassistproposal.h"

using namespace TextEditor;

/*!
    \group CodeAssist
    \title Code Assist for Editors

    Code assist is available in the form of completions and refactoring actions pop-ups
    which are triggered under particular circumstances. This group contains the classes
    used to provide such support.

    Completions can be of a variety of kind like function hints, snippets, and regular
    context-aware content. The later are usually represented by semantic proposals, but
    it is also possible that they are simply plain text like in the fake vim mode.

    Completions also have the possibility to run asynchronously in a separate thread and
    then not blocking the GUI. This is the default behavior.
*/

/*!
    \class TextEditor::IAssistProposal
    \brief The IAssistProposal class acts as an interface for representing an assist proposal.
    \ingroup CodeAssist

    Known implenters of this interface are FunctionHintProposal and GenericProposal. The
    former is recommended to be used when assisting function call constructs (overloads
    and parameters) while the latter is quite generic so that it could be used to propose
    snippets, refactoring operations (quickfixes), and contextual content (the member of
    class or a string existent in the document, for example).

    This class is part of the CodeAssist API.

    \sa IAssistProposalWidget, IAssistModel
*/

IAssistProposal::IAssistProposal()
{}

IAssistProposal::~IAssistProposal()
{}

/*!
    \fn bool TextEditor::IAssistProposal::isFragile() const

    Returns whether this is a fragile proposal. When a proposal is fragile it means that
    it will be replaced by a new proposal in the case one is created, even if due to an
    idle editor.
*/

/*!
    \fn int TextEditor::IAssistProposal::basePosition() const

    Returns the position from which this proposal starts.
*/

/*!
    \fn bool TextEditor::IAssistProposal::isCorrective() const

    Returns whether this proposal is also corrective. This could happen in C++, for example,
    when a dot operator (.) needs to be replaced by an arrow operator (->) before the proposal
    is displayed.
*/

/*!
    \fn void TextEditor::IAssistProposal::makeCorrection(BaseTextEditor *editor)

    This allows a correction to be made in the case this is a corrective proposal.
*/

/*!
    \fn IAssistModel *TextEditor::IAssistProposal::model() const

    Returns the model associated with this proposal.

    Although the IAssistModel from this proposal may be used on its own, it needs to be
    consistent with the widget returned by createWidget().

    \sa createWidget()
*/

/*!
    \fn IAssistProposalWidget *TextEditor::IAssistProposal::createWidget() const

    Returns the widget associated with this proposal. The IAssistProposalWidget implementor
    recommended for function hint proposals is FunctionHintProposalWidget. For snippets,
    refactoring operations (quickfixes), and contextual content the recommeded implementor
    is GenericProposalWidget.
*/
