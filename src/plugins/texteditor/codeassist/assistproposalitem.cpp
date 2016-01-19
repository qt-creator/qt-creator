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

void AssistProposalItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

QIcon AssistProposalItem::icon() const
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

QString AssistProposalItem::detail() const
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

bool AssistProposalItem::isSnippet() const
{
    return data().canConvert<QString>();
}

bool AssistProposalItem::isValid() const
{
    return m_data.isValid();
}

quint64 AssistProposalItem::hash() const
{
    return 0;
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

void AssistProposalItem::apply(TextDocumentManipulatorInterface &manipulator, int basePosition) const
{
    if (data().canConvert<QString>()) {
        applySnippet(manipulator, basePosition);
    } else if (data().canConvert<QuickFixOperation::Ptr>()) {
        applyQuickFix(manipulator, basePosition);
    } else {
        applyContextualContent(manipulator, basePosition);
        manipulator.encourageApply();
    }
}

void AssistProposalItem::applyContextualContent(TextDocumentManipulatorInterface &manipulator, int basePosition) const
{
    const int currentPosition = manipulator.currentPosition();
    manipulator.replace(basePosition, currentPosition - basePosition, text());

}

void AssistProposalItem::applySnippet(TextDocumentManipulatorInterface &manipulator, int basePosition) const
{
    manipulator.insertCodeSnippet(basePosition, data().toString());
}

void AssistProposalItem::applyQuickFix(TextDocumentManipulatorInterface &manipulator, int basePosition) const
{
    Q_UNUSED(manipulator)
    Q_UNUSED(basePosition)

    QuickFixOperation::Ptr op = data().value<QuickFixOperation::Ptr>();
    op->perform();
}

} // namespace TextEditor
