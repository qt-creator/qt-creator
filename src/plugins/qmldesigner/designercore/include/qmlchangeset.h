// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include <modelnode.h>
#include "qmlmodelnodefacade.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT QmlModelStateOperation : public QmlModelNodeFacade
{
public:
    QmlModelStateOperation() : QmlModelNodeFacade() {}
    QmlModelStateOperation(const ModelNode &modelNode) : QmlModelNodeFacade(modelNode) {}
    ModelNode target() const;
    void setTarget(const ModelNode &target);
    bool explicitValue() const;
    void setExplicitValue(bool value);
    bool restoreEntryValues() const;
    void setRestoreEntryValues(bool value);
    QList<AbstractProperty> targetProperties() const;
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlModelStateOperation(const ModelNode &modelNode);
};

class QMLDESIGNERCORE_EXPORT QmlPropertyChanges : public QmlModelStateOperation
{
public:
    QmlPropertyChanges() : QmlModelStateOperation() {}
    QmlPropertyChanges(const ModelNode &modelNode) : QmlModelStateOperation(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlPropertyChanges(const ModelNode &modelNode);
    void removeProperty(const PropertyName &name);
};

} //QmlDesigner
