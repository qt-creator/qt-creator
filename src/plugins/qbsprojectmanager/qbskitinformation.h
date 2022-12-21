// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitinformation.h>

namespace QbsProjectManager {
namespace Internal {

class QbsKitAspect final : public ProjectExplorer::KitAspect
{
    Q_OBJECT

public:
    QbsKitAspect();

    static QString representation(const ProjectExplorer::Kit *kit);
    static QVariantMap properties(const ProjectExplorer::Kit *kit);
    static void setProperties(ProjectExplorer::Kit *kit, const QVariantMap &properties);

private:
    static Utils::Id id();

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *) const override;
    ItemList toUserOutput(const ProjectExplorer::Kit *) const override;
    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *) const override;
};

} // namespace Internal
} // namespace QbsProjectManager
