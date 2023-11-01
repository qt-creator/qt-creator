// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include "baseqtversion.h"

#include <projectexplorer/kitaspects.h>

namespace QtSupport {

class QTSUPPORT_EXPORT QtKitAspect
{
public:
    static Utils::Id id();
    static int qtVersionId(const ProjectExplorer::Kit *k);
    static void setQtVersionId(ProjectExplorer::Kit *k, const int id);
    static QtVersion *qtVersion(const ProjectExplorer::Kit *k);
    static void setQtVersion(ProjectExplorer::Kit *k, const QtVersion *v);

    static void addHostBinariesToPath(const ProjectExplorer::Kit *k, Utils::Environment &env);

    static ProjectExplorer::Kit::Predicate platformPredicate(Utils::Id availablePlatforms);
    static ProjectExplorer::Kit::Predicate
    qtVersionPredicate(const QSet<Utils::Id> &required = QSet<Utils::Id>(),
                       const QVersionNumber &min = QVersionNumber(0, 0, 0),
                       const QVersionNumber &max = QVersionNumber(INT_MAX, INT_MAX, INT_MAX));
};

} // namespace QtSupport
