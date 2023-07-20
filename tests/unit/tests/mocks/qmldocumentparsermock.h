// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/qmldocumentparserinterface.h>

class QmlDocumentParserMock : public QmlDesigner::QmlDocumentParserInterface
{
public:
    MOCK_METHOD(QmlDesigner::Storage::Synchronization::Type,
                parse,
                (const QString &sourceContent,
                 QmlDesigner::Storage::Imports &imports,
                 QmlDesigner::SourceId sourceId,
                 Utils::SmallStringView directoryPath),
                (override));
};
