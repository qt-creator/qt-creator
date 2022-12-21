// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"
#include "qmljsdialect.h"

#include <QStringList>

#include <set>

namespace QmlJS {

struct QMLJS_EXPORT ViewerContext
{
    enum Flags {
        Complete,
        AddAllPathsAndDefaultSelectors,
        AddAllPaths,
        AddDefaultPaths,
        AddDefaultPathsAndSelectors
    };

    QStringList selectors;
    std::set<Utils::FilePath> paths;
    QList<Utils::FilePath> applicationDirectories;
    Dialect language = Dialect::Qml;
    Flags flags = AddAllPaths;
};

} // namespace QmlJS
