/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include <exewrappers/mesontools.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

namespace MesonProjectManager {
namespace Internal {
class NinjaToolKitAspect final : public ProjectExplorer::KitAspect
{
public:
    NinjaToolKitAspect();

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const final;
    void setup(ProjectExplorer::Kit *k) final;
    void fix(ProjectExplorer::Kit *k) final;
    ItemList toUserOutput(const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *) const final;

    static void setNinjaTool(ProjectExplorer::Kit *kit, Utils::Id id);
    static Utils::Id ninjaToolId(const ProjectExplorer::Kit *kit);

    static inline decltype(auto) ninjaTool(const ProjectExplorer::Kit *kit)
    {
        return MesonTools::ninjaWrapper(NinjaToolKitAspect::ninjaToolId(kit));
    }
    static inline bool isValid(const ProjectExplorer::Kit *kit)
    {
        auto tool = ninjaTool(kit);
        return (tool && tool->isValid());
    }
};

} // namespace Internal
} // namespace MesonProjectManager
