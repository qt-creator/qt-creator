// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>

#include "modelresourcemanagementfwd.h"

#include <bindingproperty.h>
#include <modelnode.h>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT ModelResourceManagementInterface
{
public:
    ModelResourceManagementInterface() = default;
    virtual ~ModelResourceManagementInterface() = default;

    ModelResourceManagementInterface(const ModelResourceManagementInterface &) = delete;
    ModelResourceManagementInterface &operator=(const ModelResourceManagementInterface &) = delete;
    ModelResourceManagementInterface(ModelResourceManagementInterface &&) = default;
    ModelResourceManagementInterface &operator=(ModelResourceManagementInterface &&) = default;

    virtual ModelResourceSet removeNodes(ModelNodes nodes, Model *model) const = 0;
    virtual ModelResourceSet removeProperties(AbstractProperties properties, Model *model) const = 0;
};
} // namespace QmlDesigner
