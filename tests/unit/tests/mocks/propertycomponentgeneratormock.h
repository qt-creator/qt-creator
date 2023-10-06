// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <propertycomponentgeneratorinterface.h>

class PropertyComponentGeneratorMock : public QmlDesigner::PropertyComponentGeneratorInterface
{
    using Property = QmlDesigner::PropertyComponentGeneratorInterface::Property;

public:
    virtual ~PropertyComponentGeneratorMock() = default;
    MOCK_METHOD(Property,
                create,
                (const QmlDesigner::PropertyMetaInfo &),

                (const, override));
    MOCK_METHOD(QStringList, imports, (), (const, override));
};
