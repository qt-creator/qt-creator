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

#include "textedititem.h"

#include <formeditorscene.h>
#include <nodemetainfo.h>

#include <QLineEdit>
#include <QTextEdit>

namespace QmlDesigner {

TextEditItem::TextEditItem(FormEditorScene* scene)
            : TextEditItemWidget(scene)
            , m_formEditorItem(0)
{
    connect(lineEdit(), &QLineEdit::returnPressed, this, &TextEditItem::returnPressed);
}

TextEditItem::~TextEditItem()
{
    m_formEditorItem = 0;
}

void TextEditItem::writeTextToProperty()
{
    if (m_formEditorItem) {
        if (text().isEmpty())
            m_formEditorItem->qmlItemNode().removeProperty("text");
        else if (m_formEditorItem->qmlItemNode().isTranslatableText("text"))
            m_formEditorItem->qmlItemNode().setBindingProperty("text", QmlObjectNode::generateTranslatableText(text()));
        else
            m_formEditorItem->qmlItemNode().setVariantProperty("text", text());
    }
}

void TextEditItem::setFormEditorItem(FormEditorItem *formEditorItem)
{
    m_formEditorItem = formEditorItem;
    QRectF rect = formEditorItem->qmlItemNode().instancePaintedBoundingRect().united(formEditorItem->qmlItemNode().instanceBoundingRect()).adjusted(-12, -4, 12 ,4);
    setGeometry(rect);

    NodeMetaInfo metaInfo = m_formEditorItem->qmlItemNode().modelNode().metaInfo();
    if (metaInfo.isValid() &&
            (metaInfo.isSubclassOf("QtQuick.TextEdit")
             || metaInfo.isSubclassOf("QtQuick.Controls.TextArea"))) {
        QSize maximumSize = rect.size().toSize();
        activateTextEdit(maximumSize);
    } else {
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
