/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    \fn bool TextEditor::IAssistProvider::supportsEditor(const QString &editorId) const

    Returns whether this provider supports the editor which has the give \a editorId.
*/

/*!
    \fn IAssistProcessor *TextEditor::IAssistProvider::createProcessor() const

    Creates and returns the IAssistProcessor responsible for computing an IAssistProposal.
*/
