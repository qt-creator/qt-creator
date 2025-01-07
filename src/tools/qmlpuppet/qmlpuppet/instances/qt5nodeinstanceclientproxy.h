// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nodeinstanceclientproxy.h"

namespace QmlDesigner {

class Qt5NodeInstanceClientProxy : public NodeInstanceClientProxy
{
    Q_OBJECT
public:
    explicit Qt5NodeInstanceClientProxy(QObject *parent = nullptr);
};

} // namespace QmlDesigner
