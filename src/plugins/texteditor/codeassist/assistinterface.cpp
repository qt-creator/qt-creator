// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <utils/textutils.h>

#include <QTextBlock>
#include <QTextDocument>
#include <QTextCursor>

#include <utils/qtcassert.h>

namespace TextEditor {

AssistInterface::AssistInterface(const QTextCursor &cursor,
                                 const Utils::FilePath &filePath,
                                 AssistReason reason)
    : m_textDocument(cursor.document())
    , m_cursor(cursor)
    , m_isAsync(false)
    , m_position(cursor.position())
    , m_anchor(cursor.anchor())
    , m_filePath(filePath)
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
    return Utils::Text::textAt(QTextCursor(m_textDocument), pos, length);
}

void AssistInterface::prepareForAsyncUse()
{
    m_text = m_textDocument->toPlainText();
    m_userStates.reserve(m_textDocument->blockCount());
    for (QTextBlock block = m_textDocument->firstBlock(); block.isValid(); block = block.next())
        m_userStates.append(block.userState());
    m_textDocument = nullptr;
    m_isAsync = true;
}

void AssistInterface::recreateTextDocument()
{
    m_textDocument = new QTextDocument(m_text);
    m_cursor = QTextCursor(m_textDocument);
    m_cursor.setPosition(m_anchor);
    m_cursor.setPosition(m_position, QTextCursor::KeepAnchor);
    m_text.clear();

    QTC_CHECK(m_textDocument->blockCount() == m_userStates.count());
    QTextBlock block = m_textDocument->firstBlock();
    for (int i = 0; i < m_userStates.count() && block.isValid(); ++i, block = block.next())
        block.setUserState(m_userStates[i]);
}

AssistReason AssistInterface::reason() const
{
    return m_reason;
}

} // namespace TextEditor
