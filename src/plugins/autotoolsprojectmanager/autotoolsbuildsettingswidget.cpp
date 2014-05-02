/**************************************************************************
**
** Copyright (C) 2014 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    m_buildConfiguration(bc)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setEnabled(true);
    m_pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_pathChooser->setBaseFileName(bc->target()->project()->projectDirectory());
    m_pathChooser->setEnvironment(bc->environment());
    m_pathChooser->setHistoryCompleter(QLatin1String("AutoTools.BuildDir.History"));
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));

    m_pathChooser->setBaseFileName(bc->target()->project()->projectDirectory());
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
    setDisplayName(tr("Autotools Manager"));

    connect(bc, SIGNAL(environmentChanged()), this, SLOT(environmentHasChanged()));
}

void AutotoolsBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(Utils::FileName::fromString(m_pathChooser->rawPath()));
}

void AutotoolsBuildSettingsWidget::environmentHasChanged()
{
    m_pathChooser->setEnvironment(m_buildConfiguration->environment());
}
