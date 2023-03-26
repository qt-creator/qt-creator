// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitinformation.h>

namespace McuSupport {
namespace Internal {

class McuDependenciesKitAspect final : public ProjectExplorer::KitAspect
{
    Q_OBJECT

public:
    McuDependenciesKitAspect();

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *kit) const override;
    void fix(ProjectExplorer::Kit *kit) override;

    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *kit) const override;

    ItemList toUserOutput(const ProjectExplorer::Kit *kit) const override;

    static Utils::Id id();
    static Utils::NameValueItems dependencies(const ProjectExplorer::Kit *kit);
    static void setDependencies(ProjectExplorer::Kit *kit, const Utils::NameValueItems &dependencies);
    static Utils::NameValuePairs configuration(const ProjectExplorer::Kit *kit);
};

} // namespace Internal
} // namespace McuSupport
