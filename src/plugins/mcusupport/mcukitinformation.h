/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
