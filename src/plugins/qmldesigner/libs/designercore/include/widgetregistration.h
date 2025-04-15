// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "widgetinfo.h"

namespace QmlDesigner {

class WidgetRegistrationInterface
{
public:
    virtual void registerWidgetInfo(WidgetInfo) = 0;
    virtual void deregisterWidgetInfo(WidgetInfo) = 0;

protected:
    ~WidgetRegistrationInterface() = default;
};

} // namespace QmlDesigner
