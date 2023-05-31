// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelresourcemanagementinterface.h"

#include <bindingproperty.h>
#include <modelnode.h>

namespace QmlDesigner {

class ModelResourceManagement final : public ModelResourceManagementInterface
{
public:
    ModelResourceSet removeNodes(ModelNodes nodes, Model *model) const override;
    ModelResourceSet removeProperties(AbstractProperties properties, Model *model) const override;
};

} // namespace QmlDesigner
