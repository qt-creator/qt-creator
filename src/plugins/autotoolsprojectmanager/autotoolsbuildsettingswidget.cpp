/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autotoolsbuildsettingswidget.h"
#include "autotoolsproject.h"
#include "autotoolsbuildconfiguration.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <utils/pathchooser.h>

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QComboBox>
#include <QPointer>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

AutotoolsBuildSettingsWidget::AutotoolsBuildSettingsWidget(AutotoolsBuildConfiguration *bc) :
    m_pathChooser(new Utils::PathChooser),
    m_buildConfiguration(bc)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_pathChooser->setEnabled(true);
    m_pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_pathChooser->setBaseFileName(bc->target()->project()->projectDirectory());
    m_pathChooser->setEnvironment(bc->environment());
    m_pathChooser->setHistoryCompleter(QLatin1String("AutoTools.BuildDir.History"));
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, &Utils::PathChooser::rawPathChanged,
            this, &AutotoolsBuildSettingsWidget::buildDirectoryChanged);

    m_pathChooser->setBaseFileName(bc->target()->project()->projectDirectory());
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
    setDisplayName(tr("Autotools Manager"));

    connect(bc, &BuildConfiguration::environmentChanged,
            this, &AutotoolsBuildSettingsWidget::environmentHasChanged);
}

void AutotoolsBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(Utils::FileName::fromString(m_pathChooser->rawPath()));
}

void AutotoolsBuildSettingsWidget::environmentHasChanged()
{
    m_pathChooser->setEnvironment(m_buildConfiguration->environment());
}
