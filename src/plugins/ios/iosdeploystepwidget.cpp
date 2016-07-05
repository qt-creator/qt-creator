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

#include "iosdeploystepwidget.h"
#include "ui_iosdeploystepwidget.h"

#include "iosdeploystep.h"
#include "iosrunconfiguration.h"

#include <coreplugin/icore.h>

#include <QFileDialog>

namespace Ios {
namespace Internal {

IosDeployStepWidget::IosDeployStepWidget(IosDeployStep *step) :
    ProjectExplorer::BuildStepConfigWidget(),
    ui(new Ui::IosDeployStepWidget),
    m_step(step)
{
    ui->setupUi(this);
    connect(m_step, &ProjectExplorer::ProjectConfiguration::displayNameChanged,
            this, &ProjectExplorer::BuildStepConfigWidget::updateSummary);
}

IosDeployStepWidget::~IosDeployStepWidget()
{
    delete ui;
}

QString IosDeployStepWidget::displayName() const
{
    return QString::fromLatin1("<b>%1</b>").arg(m_step->displayName());
}

QString IosDeployStepWidget::summaryText() const
{
    return displayName();
}

} // namespace Internal
} // namespace Ios
