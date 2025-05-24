// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlmodelnodefacade.h"
#include <modelnode.h>

namespace QmlDesigner {

class QMLDESIGNER_EXPORT QmlModelStateOperation : public QmlModelNodeFacade
{
public:
    QmlModelStateOperation() : QmlModelNodeFacade()
    {}

    QmlModelStateOperation(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
    {}

    ModelNode target(SL sl = {}) const;
    void setTarget(const ModelNode &target, SL sl = {});
    bool explicitValue(SL sl = {}) const;
    void setExplicitValue(bool value, SL sl = {});
    bool restoreEntryValues(SL sl = {}) const;
    void setRestoreEntryValues(bool value, SL sl = {});
    QList<AbstractProperty> targetProperties(SL sl = {}) const;
    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }

    static bool isValidQmlModelStateOperation(const ModelNode &modelNode, SL sl = {});
};

class QMLDESIGNER_EXPORT QmlPropertyChanges : public QmlModelStateOperation
{
public:
    QmlPropertyChanges() : QmlModelStateOperation()
    {}

    QmlPropertyChanges(const ModelNode &modelNode)
        : QmlModelStateOperation(modelNode)
    {}

    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }

    static bool isValidQmlPropertyChanges(const ModelNode &modelNode, SL sl = {});
    void removeProperty(PropertyNameView name, SL sl = {});
};

} //QmlDesigner
