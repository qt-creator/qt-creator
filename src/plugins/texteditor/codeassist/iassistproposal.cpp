/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "iassistproposal.h"

using namespace TextEditor;

/*!
    \class IAssistProposal
    \brief The IAssistProposal is an interface for representing an assist proposal.

    Known implenters of this interface are FunctionHintProposal and GenericProposal. The
    former is recommended to be used when assisting function call constructs (overloads
    and parameters) while the latter is quite generic so that it could be used to propose
    snippets, refactoring operations (quickfixes), and contextual content (the member of
    class or a string existent in the document, for example).

    \sa IAssistProposalWidget, IAssistModel
*/

IAssistProposal::IAssistProposal()
{}

IAssistProposal::~IAssistProposal()
{}

/*!
    \fn bool isFragile() const

    Returns whether this is a fragile proposal. When a proposal is fragile it means that
    it will be replaced by a new proposal in the case one is created, even if due to an
    idle editor.
*/

/*!
    \fn int basePosition() const

    Returns the position from which this proposal starts.
*/

/*!
    \fn bool isCorrective() const

    Returns whether this proposal is also corrective. This could happen in C++, for example,
    when a dot operator (.) needs to be replaced by an arrow operator (->) before the proposal
    is displayed.
*/

/*!
    \fn void makeCorrection(BaseTextEditor *editor)

    This allows a correction to be made in the case this is a corrective proposal.
*/

/*!
    \fn IAssistModel *model() const

    Returns the model associated with this proposal.

    Although the IAssistModel from this proposal may be used on its own, it needs to be
    consistent with the widget returned by createWidget().

    \sa createWidget()
*/

/*!
    \fn IAssistProposalWidget *createWidget() const

    Returns the widget associated with this proposal. The IAssistProposalWidget implementor
    recommended for function hint proposals is FunctionHintProposalWidget. For snippets,
    refactoring operations (quickfixes), and contextual content the recommeded implementor
    is GenericProposalWidget.
*/
