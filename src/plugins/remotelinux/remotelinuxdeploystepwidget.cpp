/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "remotelinuxdeploystepwidget.h"

#include "abstractremotelinuxdeploystep.h"
#include "deploymentinfo.h"
#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxutils.h"

#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4project.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
using namespace Internal;

RemoteLinuxDeployStepWidget::RemoteLinuxDeployStepWidget(AbstractRemoteLinuxDeployStep *step)
    : m_step(step)
{
    BuildStepList * const list = step->deployConfiguration()->stepList();
    connect(list, SIGNAL(stepInserted(int)), SIGNAL(updateSummary()));
    connect(list, SIGNAL(stepMoved(int,int)), SIGNAL(updateSummary()));
    connect(list, SIGNAL(stepRemoved(int)), SIGNAL(updateSummary()));
    connect(list, SIGNAL(aboutToRemoveStep(int)),
        SLOT(handleStepToBeRemoved(int)));

    connect(m_step->deployConfiguration(), SIGNAL(currentDeviceConfigurationChanged()),
        SIGNAL(updateSummary()));
}

RemoteLinuxDeployStepWidget::~RemoteLinuxDeployStepWidget()
{
}

void RemoteLinuxDeployStepWidget::handleStepToBeRemoved(int step)
{
    BuildStepList * const list = m_step->deployConfiguration()->stepList();
    const AbstractRemoteLinuxDeployStep * const alds
        = qobject_cast<AbstractRemoteLinuxDeployStep *>(list->steps().at(step));
    if (alds && alds == m_step)
        disconnect(list, 0, this, 0);
}

QString RemoteLinuxDeployStepWidget::summaryText() const
{
    return tr("<b>%1 using device</b>: %2").arg(m_step->displayName(),
        RemoteLinuxUtils::deviceConfigurationName(m_step->deployConfiguration()->deviceConfiguration()));
}

} // namespace RemoteLinux
