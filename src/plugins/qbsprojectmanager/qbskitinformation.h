// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitaspects.h>

namespace QbsProjectManager::Internal {

class QbsKitAspect final
{
public:
    static QString representation(const ProjectExplorer::Kit *kit);
    static QVariantMap properties(const ProjectExplorer::Kit *kit);
    static void setProperties(ProjectExplorer::Kit *kit, const QVariantMap &properties);

    static Utils::Id id();
};

} // QbsProjectManager::Internal
