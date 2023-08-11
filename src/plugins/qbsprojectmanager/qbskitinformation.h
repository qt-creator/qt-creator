// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitaspects.h>

namespace QbsProjectManager {
namespace Internal {

class QbsKitAspect final
{
public:
    static QString representation(const ProjectExplorer::Kit *kit);
    static QVariantMap properties(const ProjectExplorer::Kit *kit);
    static void setProperties(ProjectExplorer::Kit *kit, const QVariantMap &properties);

    static Utils::Id id();
};

class QbsKitAspectFactory final : public ProjectExplorer::KitAspectFactory
{
public:
    QbsKitAspectFactory();

private:
    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *) const override;
    ItemList toUserOutput(const ProjectExplorer::Kit *) const override;
    ProjectExplorer::KitAspect *createKitAspect(ProjectExplorer::Kit *) const override;
};

} // namespace Internal
} // namespace QbsProjectManager
