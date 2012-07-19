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

#include "basicproposalitem.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/quickfix.h>

#include <QTextCursor>

using namespace TextEditor;

BasicProposalItem::BasicProposalItem()
    : m_order(0)
{}

BasicProposalItem::~BasicProposalItem()
{}

void BasicProposalItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

const QIcon &BasicProposalItem::icon() const
{
    return m_icon;
}

void BasicProposalItem::setText(const QString &text)
{
    m_text = text;
}

QString BasicProposalItem::text() const
{
    return m_text;
}

void BasicProposalItem::setDetail(const QString &detail)
{
    m_detail = detail;
}

const QString &BasicProposalItem::detail() const
{
    return m_detail;
}

void BasicProposalItem::setData(const QVariant &var)
{
    m_data = var;
}

const QVariant &BasicProposalItem::data() const
{
    return m_data;
}

int BasicProposalItem::order() const
{
    return m_order;
}

void BasicProposalItem::setOrder(int order)
{
    m_order = order;
}

bool BasicProposalItem::implicitlyApplies() const
{
    return !data().canConvert<QString>() && !data().canConvert<QuickFixOperation::Ptr>();
}

bool BasicProposalItem::prematurelyApplies(const QChar &c) const
{
    Q_UNUSED(c);
    return false;
}

void BasicProposalItem::apply(BaseTextEditor *editor, int basePosition) const
{
    if (data().canConvert<QString>())
        applySnippet(editor, basePosition);
    else if (data().canConvert<QuickFixOperation::Ptr>())
        applyQuickFix(editor, basePosition);
    else
        applyContextualContent(editor, basePosition);
}

void BasicProposalItem::applyContextualContent(BaseTextEditor *editor, int basePosition) const
{
    const int currentPosition = editor->position();
    editor->setCursorPosition(basePosition);
    editor->replace(currentPosition - basePosition, text());
}

void BasicProposalItem::applySnippet(BaseTextEditor *editor, int basePosition) const
{
    BaseTextEditorWidget *editorWidget = static_cast<BaseTextEditorWidget *>(editor->widget());
    QTextCursor tc = editorWidget->textCursor();
    tc.setPosition(basePosition, QTextCursor::KeepAnchor);
    editorWidget->insertCodeSnippet(tc, data().toString());
}

void BasicProposalItem::applyQuickFix(BaseTextEditor *editor, int basePosition) const
{
    Q_UNUSED(editor)
    Q_UNUSED(basePosition)

    QuickFixOperation::Ptr op = data().value<QuickFixOperation::Ptr>();
    op->perform();
}
