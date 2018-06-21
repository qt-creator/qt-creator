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

#include "textitem.h"
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <qmath.h>

using namespace ScxmlEditor::PluginInterface;

TextItem::TextItem(QGraphicsItem *parent)
    : QGraphicsTextItem(parent)
{
    init();
}

TextItem::TextItem(const QString &id, QGraphicsItem *parent)
    : QGraphicsTextItem(id, parent)
{
    init();
}

void TextItem::init()
{
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFlag(ItemIsSelectable, true);
    setFlag(ItemIsFocusable, true);

    QTextOption options;
    options.setWrapMode(QTextOption::NoWrap);
    options.setAlignment(Qt::AlignCenter);
    document()->setDefaultTextOption(options);

    connect(document(), &QTextDocument::contentsChanged, this, &TextItem::checkText);

    QFont f = font();
    f.setPixelSize(13);
    setFont(f);
}

void TextItem::setItalic(bool ital)
{
    QFont f = font();
    f.setItalic(ital);
    setFont(f);
}

void TextItem::checkText()
{
    if (document()->textWidth() <= 40)
        document()->setTextWidth(40);
    else
        document()->setTextWidth(-1);

    emit textChanged();
}

void TextItem::setText(const QString &t)
{
    QSignalBlocker blocker(this);
    setPlainText(t);
}

void TextItem::focusInEvent(QFocusEvent *event)
{
    QGraphicsTextItem::focusInEvent(event);
    emit selected(true);
}

void TextItem::focusOutEvent(QFocusEvent *event)
{
    emit textReady(toPlainText());
    QGraphicsTextItem::focusOutEvent(event);
}

void TextItem::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Tab:
        event->accept();
        clearFocus();
        return;
    default:
        break;
    }

    QGraphicsTextItem::keyPressEvent(event);
}

bool TextItem::needIgnore(const QPointF sPos) const
{
    // If we found QuickTransition-item or CornerGrabber at this point, we must ignore mouse press here
    // So we can press QuickTransition/CornerGrabber item although there is transition lines front of these items
    foreach (QGraphicsItem *item, scene()->items(sPos)) {
        if (item->type() == QuickTransitionType || (item->type() == CornerGrabberType && item->parentItem() != this))
            return true;
    }

    return false;
}

void TextItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (needIgnore(event->scenePos())) {
        event->ignore();
        return;
    }

    QGraphicsTextItem::mousePressEvent(event);
    setFocus();
}

void TextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (needIgnore(event->scenePos())) {
        event->ignore();
        return;
    }

    QGraphicsTextItem::mouseReleaseEvent(event);
    setFocus();
}

void TextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (needIgnore(event->scenePos())) {
        event->ignore();
        return;
    }

    setFocus();
    QGraphicsTextItem::mouseDoubleClickEvent(event);
    emit selected(true);
}

void TextItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
    if (needIgnore(e->scenePos())) {
        e->ignore();
        return;
    }

    setCursor(Qt::IBeamCursor);
    QGraphicsTextItem::hoverEnterEvent(e);
}

void TextItem::hoverMoveEvent(QGraphicsSceneHoverEvent *e)
{
    if (needIgnore(e->scenePos())) {
        setCursor(Qt::ArrowCursor);
        e->ignore();
        return;
    }

    setCursor(Qt::IBeamCursor);
    QGraphicsTextItem::hoverEnterEvent(e);
}

void TextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
    setCursor(Qt::ArrowCursor);
    QGraphicsTextItem::hoverLeaveEvent(e);
}
