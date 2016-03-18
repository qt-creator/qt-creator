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

#pragma once

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

    QRectF boundingRect() const override;
    QPointF position() const override;
    QSizeF size() const override;
    QTransform transform() const override;
    double opacity() const override;

    void setPropertyVariant(const PropertyName &name, const QVariant &value) override;
    void setPropertyBinding(const PropertyName &name, const QString &expression) override;
    void setId(const QString &id) override;
    QVariant property(const PropertyName &name) const override;

    void initializePropertyWatcher(const ObjectNodeInstance::Pointer &objectNodeInstance);

protected:
    DummyNodeInstance();

};

}
}
