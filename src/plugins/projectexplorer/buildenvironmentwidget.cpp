/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "buildenvironmentwidget.h"

#include "buildconfiguration.h"
#include "environmentwidget.h"

#include <projectexplorer/target.h>

#include <QVBoxLayout>
#include <QCheckBox>

using namespace ProjectExplorer;

BuildEnvironmentWidget::BuildEnvironmentWidget(BuildConfiguration *bc)
    : m_buildConfiguration(0)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_clearSystemEnvironmentCheckBox = new QCheckBox(this);
    m_clearSystemEnvironmentCheckBox->setText(tr("Clear system environment"));

    m_buildEnvironmentWidget = new EnvironmentWidget(this, m_clearSystemEnvironmentCheckBox);
    vbox->addWidget(m_buildEnvironmentWidget);

    connect(m_buildEnvironmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(environmentModelUserChangesChanged()));
    connect(m_clearSystemEnvironmentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(clearSystemEnvironmentCheckBoxClicked(bool)));

    m_buildConfiguration = bc;

    connect(m_buildConfiguration->target(), SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()));

    m_clearSystemEnvironmentCheckBox->setChecked(!m_buildConfiguration->useSystemEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
    m_buildEnvironmentWidget->setUserChanges(m_buildConfiguration->userEnvironmentChanges());

    setDisplayName(tr("Build Environment"));
}

void BuildEnvironmentWidget::environmentModelUserChangesChanged()
{
    m_buildConfiguration->setUserEnvironmentChanges(m_buildEnvironmentWidget->userChanges());
}

void BuildEnvironmentWidget::clearSystemEnvironmentCheckBoxClicked(bool checked)
{
    m_buildConfiguration->setUseSystemEnvironment(!checked);
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
}

void BuildEnvironmentWidget::environmentChanged()
{
    m_buildEnvironmentWidget->setBaseEnvironment(m_buildConfiguration->baseEnvironment());
    m_buildEnvironmentWidget->setBaseEnvironmentText(m_buildConfiguration->baseEnvironmentText());
}
