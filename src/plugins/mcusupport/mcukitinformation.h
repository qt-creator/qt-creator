// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitaspects.h>

namespace McuSupport {
namespace Internal {

class McuDependenciesKitAspect final
{
public:
    static Utils::Id id();
    static Utils::NameValueItems dependencies(const ProjectExplorer::Kit *kit);
    static void setDependencies(ProjectExplorer::Kit *kit, const Utils::NameValueItems &dependencies);
    static Utils::NameValuePairs configuration(const ProjectExplorer::Kit *kit);
};

class McuDependenciesKitAspectFactory final : public ProjectExplorer::KitAspectFactory
{
public:
    McuDependenciesKitAspectFactory();

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *kit) const override;
    void fix(ProjectExplorer::Kit *kit) override;

    ProjectExplorer::KitAspect *createKitAspect(ProjectExplorer::Kit *kit) const override;

    ItemList toUserOutput(const ProjectExplorer::Kit *kit) const override;
};

} // namespace Internal
} // namespace McuSupport
