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

#include "assistproposalitem.h"

#include <texteditor/texteditor.h>
#include <texteditor/quickfix.h>

#include <QTextCursor>

namespace TextEditor {

/*!
    \class TextEditor::AssistProposalItem
    \brief The AssistProposalItem class acts as an interface for representing an assist
    proposal item.
    \ingroup CodeAssist

    This is class is part of the CodeAssist API.
*/

/*!
    \fn bool TextEditor::AssistProposalItem::implicitlyApplies() const

    Returns whether this item should implicitly apply in the case it is the only proposal
    item available.
*/

/*!
    \fn bool TextEditor::AssistProposalItem::prematurelyApplies(const QChar &c) const

    Returns whether the character \a c causes this item to be applied.
*/

/*!
    \fn void TextEditor::AssistProposalItem::apply(BaseTextEditor *editor, int basePosition) const

    This is the place to implement the actual application of the item.
*/

AssistProposalItem::AssistProposalItem()
    : m_order(0)
{}

AssistProposalItem::~AssistProposalItem()
{}

void AssistProposalItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

const QIcon &AssistProposalItem::icon() const
{
    return m_icon;
}

void AssistProposalItem::setText(const QString &text)
{
    m_text = text;
}

QString AssistProposalItem::text() const
{
    return m_text;
}

void AssistProposalItem::setDetail(const QString &detail)
{
    m_detail = detail;
}

const QString &AssistProposalItem::detail() const
{
    return m_detail;
}

void AssistProposalItem::setData(const QVariant &var)
{
    m_data = var;
}

const QVariant &AssistProposalItem::data() const
{
    return m_data;
}

int AssistProposalItem::order() const
{
    return m_order;
}

void AssistProposalItem::setOrder(int order)
{
    m_order = order;
}

bool AssistProposalItem::implicitlyApplies() const
{
    return !data().canConvert<QString>() && !data().canConvert<QuickFixOperation::Ptr>();
}

bool AssistProposalItem::prematurelyApplies(const QChar &c) const
{
    Q_UNUSED(c);
    return false;
}

void AssistProposalItem::apply(TextEditorWidget *editorWidget, int basePosition) const
{
    if (data().canConvert<QString>()) {
        applySnippet(editorWidget, basePosition);
    } else if (data().canConvert<QuickFixOperation::Ptr>()) {
        applyQuickFix(editorWidget, basePosition);
    } else {
        applyContextualContent(editorWidget, basePosition);
        editorWidget->encourageApply();
    }
}

void AssistProposalItem::applyContextualContent(TextEditorWidget *editorWidget, int basePosition) const
{
    const int currentPosition = editorWidget->position();
    editorWidget->setCursorPosition(basePosition);
    editorWidget->replace(currentPosition - basePosition, text());
}

void AssistProposalItem::applySnippet(TextEditorWidget *editorWidget, int basePosition) const
{
    QTextCursor tc = editorWidget->textCursor();
    tc.setPosition(basePosition, QTextCursor::KeepAnchor);
    editorWidget->insertCodeSnippet(tc, data().toString());
}

void AssistProposalItem::applyQuickFix(TextEditorWidget *editorWidget, int basePosition) const
{
    Q_UNUSED(editorWidget)
    Q_UNUSED(basePosition)

    QuickFixOperation::Ptr op = data().value<QuickFixOperation::Ptr>();
    op->perform();
}

} // namespace TextEditor
