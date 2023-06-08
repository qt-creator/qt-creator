// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/qmltypesparserinterface.h>

class QmlTypesParserMock : public QmlDesigner::QmlTypesParserInterface
{
public:
    MOCK_METHOD(void,
                parse,
                (const QString &sourceContent,
                 QmlDesigner::Storage::Imports &imports,
                 QmlDesigner::Storage::Synchronization::Types &types,
                 const QmlDesigner::Storage::Synchronization::ProjectData &projectData),
                (override));
};
