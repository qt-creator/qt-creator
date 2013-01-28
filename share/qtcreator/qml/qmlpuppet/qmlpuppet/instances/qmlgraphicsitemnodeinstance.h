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

#ifndef QMLGRAPHICSITEMNODEINSTANCE_H
#define QMLGRAPHICSITEMNODEINSTANCE_H

#include "graphicsobjectnodeinstance.h"
#include <QDeclarativeItem>
#include <QWeakPointer>


namespace QmlDesigner {
namespace Internal {

class QmlGraphicsItemNodeInstance : public GraphicsObjectNodeInstance
{
public:
    typedef QSharedPointer<QmlGraphicsItemNodeInstance> Pointer;
    typedef QWeakPointer<QmlGraphicsItemNodeInstance> WeakPointer;

    ~QmlGraphicsItemNodeInstance();

    static Pointer create(QObject *objectToBeWrapped);

    bool isQmlGraphicsItem() const;

    QSizeF size() const;
//    void updateAnchors();

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyBinding(const QString &name, const QString &expression);

    QVariant property(const QString &name) const;
    void resetProperty(const QString &name);

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const QString &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const QString &newParentProperty);

    int penWidth() const;

    bool hasAnchor(const QString &name) const;
    QPair<QString, ServerNodeInstance> anchor(const QString &name) const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    void doComponentComplete();

    bool isResizable() const;
    void setResizable(bool resizeable);

    void setVisible(bool isVisible);
    bool isVisible() const;

    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance);

   QList<ServerNodeInstance> stateInstances() const;

protected:
    QmlGraphicsItemNodeInstance(QDeclarativeItem *item);
    QDeclarativeItem *qmlGraphicsItem() const;
    QDeclarativeAnchors *anchors() const;
    void resetHorizontal();
    void resetVertical(); 
    void refresh();
    void recursiveDoComponentComplete(QDeclarativeItem *declarativeItem);

private: //variables
    bool m_hasHeight;
    bool m_hasWidth;
    bool m_isResizable;
    double m_x;
    double m_y;
    double m_width;
    double m_height;
};

}
}
#endif // QMLGRAPHICSITEMNODEINSTANCE_H
