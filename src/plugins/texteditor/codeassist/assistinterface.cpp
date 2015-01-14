/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "assistinterface.h"

using namespace TextEditor;

/*!
    \class TextEditor::AssistInterface
    \brief The AssistInterface class acts as an interface for providing access
    to the document from which a proposal is computed.
    \ingroup CodeAssist

    This interface existis in order to avoid a direct dependency on the text editor. This is
    particularly important and safer for asynchronous providers, since in such cases computation
    of the proposal is not done in the GUI thread.

    In general this API tries to be as decoupled as possible from the base text editor.
    This is in order to make the design a bit more generic and allow code assist to be
    pluggable into different types of documents (there are still issues to be treated).

    This class is part of the CodeAssist API.

    \sa IAssistProposal, IAssistProvider, IAssistProcessor
*/

/*!
    \fn int TextEditor::AssistInterface::position() const

    Returns the cursor position.
*/

/*!
    \fn QChar TextEditor::AssistInterface::characterAt(int position) const

    Returns the character at \a position.
*/

/*!
    \fn QString TextEditor::AssistInterface::textAt(int position, int length) const

    Returns the text at \a position with the given \a length.
*/

/*!
    \fn QString TextEditor::AssistInterface::fileName() const

    Returns the file associated.
*/

/*!
    \fn QTextDocument *TextEditor::AssistInterface::textDocument() const
    Returns the document.
*/

/*!
    \fn void TextEditor::AssistInterface::detach(QThread *destination)

    Detaches the interface. If it is necessary to take any special care in order to allow
    this interface to be run in a separate thread \a destination this needs to be done
    in this function.
*/

/*!
    \fn AssistReason TextEditor::AssistInterface::reason() const

    The reason which triggered the assist.
*/

#include "assistinterface.h"

#include <texteditor/convenience.h>

#include <QTextDocument>
#include <QTextCursor>

namespace TextEditor {

AssistInterface::AssistInterface(QTextDocument *textDocument,
                                               int position,
                                               const QString &fileName,
                                               AssistReason reason)
    : m_textDocument(textDocument)
    , m_isAsync(false)
    , m_position(position)
    , m_fileName(fileName)
    , m_reason(reason)
{}

AssistInterface::~AssistInterface()
{
    if (m_isAsync)
        delete m_textDocument;
}

QChar AssistInterface::characterAt(int position) const
{
    return m_textDocument->characterAt(position);
}

QString AssistInterface::textAt(int pos, int length) const
{
    return Convenience::textAt(QTextCursor(m_textDocument), pos, length);
}

void AssistInterface::prepareForAsyncUse()
{
    m_text = m_textDocument->toPlainText();
    m_textDocument = 0;
    m_isAsync = true;
}

void AssistInterface::recreateTextDocument()
{
    m_textDocument = new QTextDocument(m_text);
    m_text.clear();
}

AssistReason AssistInterface::reason() const
{
    return m_reason;
}

} // namespace TextEditor
