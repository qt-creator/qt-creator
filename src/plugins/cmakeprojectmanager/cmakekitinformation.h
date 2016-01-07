/****************************************************************************
**
** Copyright (C) 2015 Canonical Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef CMAKEKITINFORMATION_H
#define CMAKEKITINFORMATION_H

#include "cmake_global.h"

#include <projectexplorer/kitmanager.h>

namespace CMakeProjectManager {

class CMakeTool;

class CMAKE_EXPORT CMakeKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT
public:
    CMakeKitInformation();

    static Core::Id id();

    static CMakeTool *cmakeTool(const ProjectExplorer::Kit *k);
    static void setCMakeTool(ProjectExplorer::Kit *k, const Core::Id id);
    static Core::Id defaultValue();

    // KitInformation interface
    QVariant defaultValue(ProjectExplorer::Kit *) const override;
    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *k) const override;
    void setup(ProjectExplorer::Kit *k) override;
    void fix(ProjectExplorer::Kit *k) override;
    ItemList toUserOutput(const ProjectExplorer::Kit *k) const override;
    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *k) const override;
};

} // namespace CMakeProjectManager

#endif // CMAKEKITINFORMATION_H
