// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include "modelnode.h"

namespace QmlDesigner {

class AbstractView;

class MaterialUtils
{
public:
    MaterialUtils();

    static void assignMaterialTo3dModel(AbstractView *view, const ModelNode &modelNode,
                                        const ModelNode &materialNode = {});
};

} // namespace QmlDesigner
