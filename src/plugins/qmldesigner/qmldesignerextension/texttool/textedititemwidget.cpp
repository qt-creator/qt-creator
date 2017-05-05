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

#include <utils/theme/theme.h>

#include <QLineEdit>
#include <QGraphicsScene>
#include <QPainter>
#include <QTextEdit>

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

void TextEditItemWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->fillRect(boundingRect(), Qt::white);

    /* Cursor painting is broken.
     * QGraphicsProxyWidget::paint(painter, option, widget);
     * We draw manually instead.
    */

    QPixmap pixmap = widget()->grab();
    painter->drawPixmap(0, 0, pixmap);
}

QLineEdit* TextEditItemWidget::lineEdit() const
{
    if (m_lineEdit.isNull()) {
        m_lineEdit.reset(new QLineEdit);
        m_lineEdit->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        QPalette palette = m_lineEdit->palette();
        static QColor selectionColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorSelectionColor);
        palette.setColor(QPalette::Highlight, selectionColor);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::Text, Qt::black);
        m_lineEdit->setPalette(palette);
    }
    return m_lineEdit.data();
}

QTextEdit* TextEditItemWidget::textEdit() const
{
    if (m_textEdit.isNull()) {
        m_textEdit.reset(new QTextEdit);
        QPalette palette = m_textEdit->palette();
        static QColor selectionColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorSelectionColor);
        palette.setColor(QPalette::Highlight, selectionColor);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::Text, Qt::black);
        m_textEdit->setPalette(palette);
    }

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
