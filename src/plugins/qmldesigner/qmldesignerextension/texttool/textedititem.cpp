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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "textedititem.h"

#include <formeditorscene.h>
#include <QTextEdit>
#include <QLineEdit>
#include <nodemetainfo.h>


namespace QmlDesigner {

TextEditItem::TextEditItem(FormEditorScene* scene)
            : QGraphicsProxyWidget(),
              m_lineEdit(new QLineEdit),
              m_textEdit(new QTextEdit),
              m_formEditorItem(0)
{
    scene->addItem(this);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    setWidget(m_lineEdit.data());
    m_lineEdit->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_lineEdit->setFocus();
    connect(m_lineEdit.data(), &QLineEdit::returnPressed, this, &TextEditItem::returnPressed);
}

TextEditItem::~TextEditItem()
{
    setWidget(0);
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

QString TextEditItem::text() const
{
    if (widget() == m_lineEdit.data())
        return m_lineEdit->text();
    else if (widget() == m_textEdit.data())
        return m_textEdit->toPlainText();
    return QString();
}

void TextEditItem::setFormEditorItem(FormEditorItem *formEditorItem)
{
    m_formEditorItem = formEditorItem;
    QRectF rect = formEditorItem->qmlItemNode().instancePaintedBoundingRect().united(formEditorItem->qmlItemNode().instanceBoundingRect()).adjusted(-12, -4, 12 ,4);
    setGeometry(rect);
    m_textEdit->setMaximumSize(rect.size().toSize());

    NodeMetaInfo metaInfo = m_formEditorItem->qmlItemNode().modelNode().metaInfo();
    if (metaInfo.isValid() &&
            (metaInfo.isSubclassOf("QtQuick.TextEdit", -1, -1)
             || metaInfo.isSubclassOf("QtQuick.Controls.TextArea", -1, -1))) {
        setWidget(m_textEdit.data());
        m_textEdit->setFocus();
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
        if (widget() == m_lineEdit.data()) {
            m_lineEdit->setText(formEditorItem()->qmlItemNode().stripedTranslatableText("text"));
            m_lineEdit->selectAll();
        } else if (widget() == m_textEdit.data()) {
            m_textEdit->setText(formEditorItem()->qmlItemNode().stripedTranslatableText("text"));
            m_textEdit->selectAll();
        }
    }
}


}
