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

#ifndef QuickITEMNODEINSTANCE_H
#define QuickITEMNODEINSTANCE_H

#include <QtGlobal>

#include "graphicalnodeinstance.h"

#include <QQuickItem>
#include <designersupport.h>

namespace QmlDesigner {
namespace Internal {

class QuickItemNodeInstance : public GraphicalNodeInstance
{
public:
    typedef QSharedPointer<QuickItemNodeInstance> Pointer;
    typedef QWeakPointer<QuickItemNodeInstance> WeakPointer;

    ~QuickItemNodeInstance();

    static Pointer create(QObject *objectToBeWrapped);

    virtual QQuickItem *contentItem() const;

    QRectF contentItemBoundingBox() const Q_DECL_OVERRIDE;

    QTransform transform() const Q_DECL_OVERRIDE;
    QTransform contentItemTransform() const Q_DECL_OVERRIDE;

    QObject *parent() const Q_DECL_OVERRIDE;

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty) Q_DECL_OVERRIDE;

    bool isAnchoredBySibling() const Q_DECL_OVERRIDE;
    bool isResizable() const Q_DECL_OVERRIDE;
    bool isMovable() const Q_DECL_OVERRIDE;
    bool isQuickItem() const Q_DECL_OVERRIDE;

    void doComponentComplete();

protected:
    QuickItemNodeInstance(QQuickItem*);
    QQuickItem *quickItem() const;
    void setMovable(bool movable);
    void setResizable(bool resizable);

private: //variables
    QPointer<QQuickItem> m_contentItem;
    bool m_isResizable;
    bool m_isMovable;
};

} // namespace Internal
} // namespace QmlDesigner

#endif  // QuickITEMNODEINSTANCE_H

