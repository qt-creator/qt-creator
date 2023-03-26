// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textedititem.h"

#include <externaldependenciesinterface.h>
#include <formeditorscene.h>
#include <formeditorview.h>
#include <model.h>
#include <nodemetainfo.h>
#include <rewritingexception.h>

#include <QLineEdit>
#include <QTextEdit>

namespace QmlDesigner {

TextEditItem::TextEditItem(FormEditorScene* scene)
            : TextEditItemWidget(scene)
            , m_formEditorItem(nullptr)
{
    connect(lineEdit(), &QLineEdit::returnPressed, this, &TextEditItem::returnPressed);
}

TextEditItem::~TextEditItem()
{
    m_formEditorItem = nullptr;
}

void TextEditItem::writeTextToProperty()
{
    if (m_formEditorItem) {
        try {
            if (text().isEmpty())
                m_formEditorItem->qmlItemNode().removeProperty("text");
            else if (m_formEditorItem->qmlItemNode().isTranslatableText("text"))
                m_formEditorItem->qmlItemNode().setBindingProperty(
                    "text",
                    QmlObjectNode::generateTranslatableText(
                        text(),
                        m_formEditorItem->formEditorView()->externalDependencies().designerSettings()));
            else
                m_formEditorItem->qmlItemNode().setVariantProperty("text", text());
        }
        catch (const RewritingException &e) {
            e.showException();
        }
    }
}

void TextEditItem::setFormEditorItem(FormEditorItem *formEditorItem)
{
    m_formEditorItem = formEditorItem;
    QRectF rect = formEditorItem->qmlItemNode().instancePaintedBoundingRect().united(formEditorItem->qmlItemNode().instanceBoundingRect()).adjusted(-12, -4, 12 ,4);
    setGeometry(rect);

    NodeMetaInfo metaInfo = m_formEditorItem->qmlItemNode().modelNode().metaInfo();
    auto node = m_formEditorItem->qmlItemNode();
    auto font = node.instanceValue("font").value<QFont>();
    auto model = node.modelNode().model();
    if (metaInfo.isBasedOn(model->qtQuickTextEditMetaInfo(),
                           model->qtQuickControlsTextAreaMetaInfo())) {
        QSize maximumSize = rect.size().toSize();
        textEdit()->setFont(font);
        activateTextEdit(maximumSize);
    } else {
        auto lineEdit = TextEditItemWidget::lineEdit();
        lineEdit->setFont(font);
        activateLineEdit();
    }

    setTransform(formEditorItem->sceneTransform());
    updateText();
}

FormEditorItem *TextEditItem::formEditorItem() const
{
    return m_formEditorItem;
}

void TextEditItem::updateText()
{
    if (formEditorItem()) {
        TextEditItemWidget::updateText(formEditorItem()->qmlItemNode().
            stripedTranslatableText("text"));
    }
}
} // namespace QmlDesigner
