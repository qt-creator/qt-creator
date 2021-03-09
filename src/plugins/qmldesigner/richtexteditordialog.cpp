/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "richtexteditordialog.h"
#include <QVBoxLayout>

namespace QmlDesigner {

RichTextEditorDialog::RichTextEditorDialog(QString text)
{
    m_editor = new RichTextEditor(this);
    m_editor->setRichText(text);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);
    setLayout(layout);

    connect(m_editor, &RichTextEditor::textChanged,
            this, &RichTextEditorDialog::onTextChanged);

    connect(this, &QDialog::finished,
            this, &RichTextEditorDialog::onFinished);

    setModal(true);
}

void RichTextEditorDialog::setFormEditorItem(FormEditorItem* formEditorItem)
{
    m_formEditorItem = formEditorItem;
}

void RichTextEditorDialog::onTextChanged(QString text)
{
    Q_UNUSED(text);
    // TODO: try adding following and make it react faster
    // setTextToFormEditorItem(text);
}

void RichTextEditorDialog::onFinished()
{
    setTextToFormEditorItem(m_editor->richText());
}

void RichTextEditorDialog::setTextToFormEditorItem(QString text)
{
    if (m_formEditorItem) {
        if (text.isEmpty())
            m_formEditorItem->qmlItemNode().removeProperty("text");
        else
            m_formEditorItem->qmlItemNode().setVariantProperty("text", text);
    }
}

} //namespace QmlDesigner
