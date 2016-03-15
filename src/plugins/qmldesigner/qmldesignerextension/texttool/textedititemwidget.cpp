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
#include "textedititemwidget.h"

#include <QTextEdit>
#include <QLineEdit>
#include <QGraphicsScene>

namespace QmlDesigner {

TextEditItemWidget::TextEditItemWidget(QGraphicsScene* scene)
    : QGraphicsProxyWidget()
{
    scene->addItem(this);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    activateLineEdit();
}

TextEditItemWidget::~TextEditItemWidget()
{
    setWidget(0);
}

QLineEdit* TextEditItemWidget::lineEdit() const
{
    if (m_lineEdit.isNull()) {
        m_lineEdit.reset(new QLineEdit);
        m_lineEdit->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    }
    return m_lineEdit.data();
}

QTextEdit* TextEditItemWidget::textEdit() const
{
    if (m_textEdit.isNull())
        m_textEdit.reset(new QTextEdit);
    return m_textEdit.data();
}

void TextEditItemWidget::activateTextEdit(const QSize &maximumSize)
{
    textEdit()->setMaximumSize(maximumSize);
    textEdit()->setFocus();
    setWidget(textEdit());
}

void TextEditItemWidget::activateLineEdit()
{
    lineEdit()->setFocus();
    setWidget(lineEdit());
}

QString TextEditItemWidget::text() const
{
    if (widget() == m_lineEdit.data())
        return m_lineEdit->text();
    else if (widget() == m_textEdit.data())
        return m_textEdit->toPlainText();
    return QString();
}

void TextEditItemWidget::updateText(const QString &text)
{
    if (widget() == m_lineEdit.data()) {
        m_lineEdit->setText(text);
        m_lineEdit->selectAll();
    } else if (widget() == m_textEdit.data()) {
        m_textEdit->setText(text);
        m_textEdit->selectAll();
    }
}
} // namespace QmlDesigner
