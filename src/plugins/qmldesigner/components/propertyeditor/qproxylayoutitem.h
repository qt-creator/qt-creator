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

#ifndef QPROXYLAYOUTITEM_H
#define QPROXYLAYOUTITEM_H

#include <qdeclarative.h>
#include <QGraphicsLayout>

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QProxyLayout : public QObject, public QGraphicsLayout
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsLayout QGraphicsLayoutItem)
    Q_PROPERTY(QGraphicsLayout *layout READ layout WRITE setLayout)
public:
    QProxyLayout(QObject *parent=0);

    void setLayout(QGraphicsLayout *);
    QGraphicsLayout *layout() const;

    virtual void setGeometry(const QRectF &);
    virtual int count() const;
    virtual QGraphicsLayoutItem *itemAt(int) const;
    virtual void removeAt(int);
    virtual void updateGeometry();

protected:
    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const;

private:
    QRectF geometry;
    QGraphicsLayout *proxy;
};

class QProxyLayoutItem : public QObject, public QGraphicsLayoutItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsLayoutItem)
    Q_PROPERTY(QGraphicsLayoutItem *widget READ item WRITE setItem)
public:
    QProxyLayoutItem(QGraphicsLayoutItem * = 0);

    virtual void setGeometry(const QRectF &);

    QGraphicsLayoutItem *item() const;
    void setItem(QGraphicsLayoutItem *);

    void setEnabled(bool);

    static void registerDeclarativeTypes();

protected:
    virtual QSizeF sizeHint(Qt::SizeHint, const QSizeF &) const;

private:
    bool enabled;
    QRectF geometry;
    QGraphicsLayoutItem *other;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QProxyLayout)
QML_DECLARE_TYPE(QProxyLayoutItem)

#endif // QPROXYLAYOUTITEM_H

