// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../include/qmldesignercorelib_exports.h"

#include <bindingproperty.h>

namespace QmlDesigner {

struct ModelResourceSet
{
    struct SetExpression
    {
        BindingProperty property;
        QString expression;
    };

    using SetExpressions = QList<SetExpression>;

    QList<ModelNode> removeModelNodes;
    QList<AbstractProperty> removeProperties;
    QList<SetExpression> setExpressions;
};

} // namespace QmlDesigner
