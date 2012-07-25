/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

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

AutotoolsBuildSettingsWidget::AutotoolsBuildSettingsWidget() :
    m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setEnabled(true);
    m_pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));
}

QString AutotoolsBuildSettingsWidget::displayName() const
{
    return QLatin1String("Autotools Manager");
}

void AutotoolsBuildSettingsWidget::init(BuildConfiguration *bc)
{
    m_buildConfiguration = static_cast<AutotoolsBuildConfiguration *>(bc);
    m_pathChooser->setBaseDirectory(bc->target()->project()->projectDirectory());
    m_pathChooser->setPath(m_buildConfiguration->buildDirectory());
}

void AutotoolsBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(m_pathChooser->rawPath());
}
