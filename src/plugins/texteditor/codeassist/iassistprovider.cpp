// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    \fn bool TextEditor::IAssistProvider::supportsEditor(Utils::Id editorId) const

    Returns whether this provider supports the editor which has the \a editorId.
*/

/*!
    \fn IAssistProcessor *TextEditor::IAssistProvider::createProcessor() const

    Creates and returns the IAssistProcessor responsible for computing an IAssistProposal.
*/
