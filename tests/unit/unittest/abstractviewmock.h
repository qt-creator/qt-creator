// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <googletest.h>

#include <qmldesigner/designercore/include/abstractview.h>

class AbstractViewMock : public QmlDesigner::AbstractView
{
public:
    MOCK_METHOD(void, nodeOrderChanged, (const QmlDesigner::NodeListProperty &listProperty), (override));
};
