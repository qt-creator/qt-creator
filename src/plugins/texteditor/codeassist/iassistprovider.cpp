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

#include "iassistprovider.h"

using namespace TextEditor;

/*!
    \class TextEditor::IAssistProvider
    \brief The IAssistProvider class acts as an interface for providing code assist.
    \ingroup CodeAssist

    There might be different kinds of assist such as completions or refactoring
    actions (quickfixes).

    Within this API the term completion denotes any kind of information prompted
    to the user in order to auxiliate her to "complete" a particular code construction.
    Examples of completions currently supported are snippets, function hints, and
    contextual contents.

    This is class is part of the CodeAssist API.

    \sa IAssistProposal, IAssistProcessor
*/

/*!
    \fn bool TextEditor::IAssistProvider::isAsynchronous() const;

    Returns whether this provider runs asynchronously.
*/

/*!
    \fn bool TextEditor::IAssistProvider::supportsEditor(Core::Id editorId) const

    Returns whether this provider supports the editor which has the \a editorId.
*/

/*!
    \fn IAssistProcessor *TextEditor::IAssistProvider::createProcessor() const

    Creates and returns the IAssistProcessor responsible for computing an IAssistProposal.
*/
