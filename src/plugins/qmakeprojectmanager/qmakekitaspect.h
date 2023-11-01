// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitmanager.h>

namespace QmakeProjectManager::Internal {

class QmakeKitAspect
{
public:
    static Utils::Id id();
    enum class MkspecSource { User, Code };
    static void setMkspec(ProjectExplorer::Kit *k, const QString &mkspec, MkspecSource source);
    static QString mkspec(const ProjectExplorer::Kit *k);
    static QString effectiveMkspec(const ProjectExplorer::Kit *k);
    static QString defaultMkspec(const ProjectExplorer::Kit *k);
};

} // QmakeProjectManager::Internal
