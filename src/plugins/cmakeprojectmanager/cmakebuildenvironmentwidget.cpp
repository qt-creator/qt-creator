/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cmakebuildenvironmentwidget.h"
#include "cmakeproject.h"
#include <projectexplorer/environmenteditmodel.h>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>

namespace {
bool debug = false;
}

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeBuildEnvironmentWidget::CMakeBuildEnvironmentWidget(CMakeProject *project)
    : BuildConfigWidget(), m_pro(project)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_clearSystemEnvironmentCheckBox = new QCheckBox(this);
    m_clearSystemEnvironmentCheckBox->setText(tr("Clear system environment"));

    m_buildEnvironmentWidget = new ProjectExplorer::EnvironmentWidget(this, m_clearSystemEnvironmentCheckBox);
    vbox->addWidget(m_buildEnvironmentWidget);

    connect(m_buildEnvironmentWidget, SIGNAL(userChangesUpdated()),
            this, SLOT(environmentModelUserChangesUpdated()));
    connect(m_buildEnvironmentWidget, SIGNAL(detailsVisibleChanged(bool)),
            this, SLOT(layoutFixup()));
    connect(m_clearSystemEnvironmentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(clearSystemEnvironmentCheckBoxClicked(bool)));
}

void CMakeBuildEnvironmentWidget::layoutFixup()
{
    fixupLayout(m_buildEnvironmentWidget->detailsWidget());
}

QString CMakeBuildEnvironmentWidget::displayName() const
{
    return tr("Build Environment");
}

void CMakeBuildEnvironmentWidget::init(const QString &buildConfigurationName)
{
    if (debug)
        qDebug() << "Qt4BuildConfigWidget::init()";

    m_buildConfiguration = buildConfigurationName;

    ProjectExplorer::BuildConfiguration *bc = m_pro->buildConfiguration(buildConfigurationName);
    m_clearSystemEnvironmentCheckBox->setChecked(!m_pro->useSystemEnvironment(bc));
    m_buildEnvironmentWidget->setBaseEnvironment(m_pro->baseEnvironment(bc));
    m_buildEnvironmentWidget->setUserChanges(m_pro->userEnvironmentChanges(bc));
    m_buildEnvironmentWidget->updateButtons();
}

void CMakeBuildEnvironmentWidget::environmentModelUserChangesUpdated()
{
    m_pro->setUserEnvironmentChanges(
            m_pro->buildConfiguration(m_buildConfiguration), m_buildEnvironmentWidget->userChanges());
}

void CMakeBuildEnvironmentWidget::clearSystemEnvironmentCheckBoxClicked(bool checked)
{
    ProjectExplorer::BuildConfiguration *bc = m_pro->buildConfiguration(m_buildConfiguration);
    m_pro->setUseSystemEnvironment(bc, !checked);
    m_buildEnvironmentWidget->setBaseEnvironment(m_pro->baseEnvironment(bc));
}
