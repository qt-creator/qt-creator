/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtGlobal>

#include "objectnodeinstance.h"

#include <designersupportdelegate.h>

QT_FORWARD_DECLARE_CLASS(QQuick3DNode)

namespace QmlDesigner {
namespace Internal {

class Quick3DNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<Quick3DNodeInstance>;

    ~Quick3DNodeInstance() override;
    static Pointer create(QObject *objectToBeWrapped);
    void setHideInEditor(bool b) override;
    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                    InstanceContainer::NodeFlags flags) override;

protected:
    explicit Quick3DNodeInstance(QObject *node);

private:
    Qt5NodeInstanceServer *qt5NodeInstanceServer() const;
    QQuick3DNode *quick3DNode() const;
    void setPickable(bool enable, bool checkParent, bool applyToChildInstances);
};

} // namespace Internal
} // namespace QmlDesigner
