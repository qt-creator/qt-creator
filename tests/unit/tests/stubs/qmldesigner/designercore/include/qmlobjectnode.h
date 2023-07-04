// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlmodelnodefacade.h"
#include <qmldesignercorelib_global.h>

#include <nodeinstance.h>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT QmlObjectNode : public QmlModelNodeFacade
{
public:
    QmlObjectNode() {}
    QmlObjectNode([[maybe_unused]] const ModelNode &modelNode){};
};

} // namespace QmlDesigner
