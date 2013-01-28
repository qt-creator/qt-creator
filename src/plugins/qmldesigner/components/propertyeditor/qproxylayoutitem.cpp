/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qproxylayoutitem.h"
#include <QGraphicsWidget>

QT_BEGIN_NAMESPACE

QProxyLayoutItem::QProxyLayoutItem(QGraphicsLayoutItem *i)
: enabled(true), other(i)
{
}

void QProxyLayoutItem::setGeometry(const QRectF &r)
{
    geometry = r;
    if (enabled && other)
        other->setGeometry(r);
}

QGraphicsLayoutItem *QProxyLayoutItem::item() const
{
    return other;
}

void QProxyLayoutItem::setItem(QGraphicsLayoutItem *o)
{
    if (other == o)
        return;
    other = o;
    if (enabled && other)
        other->setGeometry(geometry);

    updateGeometry();
    if (parentLayoutItem())
        parentLayoutItem()->updateGeometry();
}

void QProxyLayoutItem::setEnabled(bool e)
{
    if (e == enabled)
        return;

    enabled = e;
    if (e && other)
        other->setGeometry(geometry);
}

QSizeF QProxyLayoutItem::sizeHint(Qt::SizeHint which, const QSizeF &c) const
{
    struct Accessor : public QGraphicsLayoutItem
    {
        QSizeF getSizeHint(Qt::SizeHint which, const QSizeF &c) const
        {
            return sizeHint(which, c);
        }
    };

    QSizeF rv;
    if (other)
        rv = static_cast<Accessor *>(other)->getSizeHint(which, c);
    return rv;
}

QProxyLayout::QProxyLayout(QObject *parent)
: QObject(parent), proxy(0)
{
}

void QProxyLayout::setLayout(QGraphicsLayout *l)
{
    proxy = l;
    updateGeometry();
    if (parentLayoutItem())
        parentLayoutItem()->updateGeometry();
}

QGraphicsLayout *QProxyLayout::layout() const
{
    return proxy;
}

void QProxyLayout::updateGeometry()
{
    QGraphicsLayout::updateGeometry();
}

void QProxyLayout::setGeometry(const QRectF &g)
{
    geometry = g;
    if (proxy)
        proxy->setGeometry(g);

}

int QProxyLayout::count() const
{
    if (proxy)
        return proxy->count();
    else
        return 0;
}

QGraphicsLayoutItem *QProxyLayout::itemAt(int idx) const
{
    if (proxy)
        return proxy->itemAt(idx);
    else
        return 0;
}

void QProxyLayout::removeAt(int idx)
{
    if (proxy)
        proxy->removeAt(idx);
}

QSizeF QProxyLayout::sizeHint(Qt::SizeHint which,
                              const QSizeF &constraint) const
{
    struct Accessor : public QGraphicsLayout
    {
        QSizeF getSizeHint(Qt::SizeHint which, const QSizeF &c) const
        {
            return sizeHint(which, c);
        }
    };

    if (proxy)
        return static_cast<Accessor *>(proxy)->getSizeHint(which, constraint);
    else
        return QSizeF();
}

void QProxyLayoutItem::registerDeclarativeTypes()
{
    qmlRegisterType<QProxyLayoutItem>("Bauhaus",1,0,"LayoutItem");
    qmlRegisterType<QProxyLayout>("Bauhaus",1,0,"ProxyLayout");
}

QT_END_NAMESPACE
