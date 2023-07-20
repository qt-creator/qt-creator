// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstoragetypes.h"

#include <QString>

namespace QmlDesigner {

class QmlTypesParserInterface
{
public:
    virtual void parse(const QString &sourceContent,
                       Storage::Imports &imports,
                       Storage::Synchronization::Types &types,
                       const Storage::Synchronization::ProjectData &projectData)
        = 0;

protected:
    ~QmlTypesParserInterface() = default;
};
} // namespace QmlDesigner
