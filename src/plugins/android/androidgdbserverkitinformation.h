/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

namespace Android {
namespace Internal {

class AndroidGdbServerKitAspect : public ProjectExplorer::KitAspect
{
    Q_OBJECT
public:
    AndroidGdbServerKitAspect();

    void setup(ProjectExplorer::Kit *) override;
    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *) const override;
    bool isApplicableToKit(const ProjectExplorer::Kit *k) const override;
    ItemList toUserOutput(const ProjectExplorer::Kit *) const override;

    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *) const override;

    static Core::Id id();
    static Utils::FilePath gdbServer(const ProjectExplorer::Kit *kit);
    static void setGdbSever(ProjectExplorer::Kit *kit, const Utils::FilePath &gdbServerCommand);
    static Utils::FilePath autoDetect(const ProjectExplorer::Kit *kit);
};

} // namespace Internal
} // namespace Android
