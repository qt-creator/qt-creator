// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

void RichTextEditorDialog::onTextChanged([[maybe_unused]] QString text)
{
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
