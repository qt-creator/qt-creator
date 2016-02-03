/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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

#include "cmake_global.h"

#include <projectexplorer/kitmanager.h>

namespace CMakeProjectManager {

class CMAKE_EXPORT CMakePreloadCacheKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT
public:
    CMakePreloadCacheKitInformation();

    static Core::Id id();

    // KitInformation interface
    QVariant defaultValue(const ProjectExplorer::Kit *) const override;
    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *k) const override;
    void setup(ProjectExplorer::Kit *k) override;
    void fix(ProjectExplorer::Kit *k) override;
    ItemList toUserOutput(const ProjectExplorer::Kit *k) const override;
    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *k) const override;

    static void setPreloadCacheFile(ProjectExplorer::Kit *k, const Utils::FileName &preload);
    static Utils::FileName preloadCacheFile(const ProjectExplorer::Kit *k);
};

} // namespace CMakeProjectManager
