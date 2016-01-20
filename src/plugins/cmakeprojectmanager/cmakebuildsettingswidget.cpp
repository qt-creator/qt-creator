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

#include "cmakebuildsettingswidget.h"
#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QFormLayout>

namespace CMakeProjectManager {
namespace Internal {

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) :
    m_buildConfiguration(bc)
{
    auto vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    auto container = new Utils::DetailsWidget;
    container->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(container);

    auto details = new QWidget(container);
    container->setWidget(details);

    auto fl = new QFormLayout(details);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto project = static_cast<CMakeProject *>(bc->target()->project());
    auto buildDirChooser = new Utils::PathChooser;
    buildDirChooser->setBaseFileName(project->projectDirectory());
    buildDirChooser->setFileName(bc->buildDirectory());
    connect(buildDirChooser, &Utils::PathChooser::rawPathChanged, this,
            [this, project](const QString &path) {
                project->changeBuildDirectory(m_buildConfiguration, path);
            });

    fl->addRow(tr("Build directory:"), buildDirChooser);

    setDisplayName(tr("CMake"));
}

} // namespace Internal
} // namespace CMakeProjectManager
