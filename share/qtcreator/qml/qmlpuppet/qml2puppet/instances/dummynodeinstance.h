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

#ifndef DUMMYNODEINSTANCE_H
#define DUMMYNODEINSTANCE_H

#include <QWeakPointer>

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class DummyNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<DummyNodeInstance> Pointer;
    typedef QWeakPointer<DummyNodeInstance> WeakPointer;

    static Pointer create();

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QPointF position() const Q_DECL_OVERRIDE;
    QSizeF size() const Q_DECL_OVERRIDE;
    QTransform transform() const Q_DECL_OVERRIDE;
    double opacity() const Q_DECL_OVERRIDE;

    void setPropertyVariant(const PropertyName &name, const QVariant &value) Q_DECL_OVERRIDE;
    void setPropertyBinding(const PropertyName &name, const QString &expression) Q_DECL_OVERRIDE;
    void setId(const QString &id) Q_DECL_OVERRIDE;
    QVariant property(const PropertyName &name) const Q_DECL_OVERRIDE;

    void initializePropertyWatcher(const ObjectNodeInstance::Pointer &objectNodeInstance);

protected:
    DummyNodeInstance();

};

}
}
#endif // DUMMYNODEINSTANCE_H
