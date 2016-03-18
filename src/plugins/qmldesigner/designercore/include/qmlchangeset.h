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

#include <qmldesignercorelib_global.h>
#include <modelnode.h>
#include "qmlmodelnodefacade.h"

namespace QmlDesigner {

class  QMLDESIGNERCORE_EXPORT QmlModelStateOperation : public QmlModelNodeFacade
{
public:
    QmlModelStateOperation() : QmlModelNodeFacade() {}
    QmlModelStateOperation(const ModelNode &modelNode) : QmlModelNodeFacade(modelNode) {}
    ModelNode target() const;
    void setTarget(const ModelNode &target);
    bool isValid() const;
    static bool isValidQmlModelStateOperation(const ModelNode &modelNode);
};


class  QMLDESIGNERCORE_EXPORT QmlPropertyChanges : public QmlModelStateOperation
{
public:
    QmlPropertyChanges() : QmlModelStateOperation() {}
    QmlPropertyChanges(const ModelNode &modelNode) : QmlModelStateOperation(modelNode) {}
    bool isValid() const;
    static bool isValidQmlPropertyChanges(const ModelNode &modelNode);
    void removeProperty(const PropertyName &name);
};

} //QmlDesigner
