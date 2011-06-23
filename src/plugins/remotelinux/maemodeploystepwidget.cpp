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

#include "maemodeploystepwidget.h"

#include "abstractmaemodeploystep.h"
#include "maemoglobal.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QtGui/QVBoxLayout>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

MaemoDeployStepBaseWidget::MaemoDeployStepBaseWidget(AbstractLinuxDeviceDeployStep *step)
    : m_step(step)
{
    BuildStepList * const list = step->maemoDeployConfig()->stepList();
    connect(list, SIGNAL(stepInserted(int)), SIGNAL(updateSummary()));
    connect(list, SIGNAL(stepMoved(int,int)), SIGNAL(updateSummary()));
    connect(list, SIGNAL(stepRemoved(int)), SIGNAL(updateSummary()));
    connect(list, SIGNAL(aboutToRemoveStep(int)),
        SLOT(handleStepToBeRemoved(int)));

    connect(m_step->maemoDeployConfig(), SIGNAL(currentDeviceConfigurationChanged()),
        SIGNAL(updateSummary()));
}

MaemoDeployStepBaseWidget::~MaemoDeployStepBaseWidget()
{
}

void MaemoDeployStepBaseWidget::handleStepToBeRemoved(int step)
{
    BuildStepList * const list = m_step->maemoDeployConfig()->stepList();
    const AbstractLinuxDeviceDeployStep * const alds
        = dynamic_cast<AbstractLinuxDeviceDeployStep *>(list->steps().at(step));
    if (alds && alds == m_step)
        disconnect(list, 0, this, 0);
}

QString MaemoDeployStepBaseWidget::summaryText() const
{
    QString error;
    if (!m_step->isDeploymentPossible(error)) {
        return QLatin1String("<font color=\"red\">")
            + tr("Cannot deploy: %1").arg(error)
            + QLatin1String("</font>");
    }
    return tr("<b>%1 using device</b>: %2").arg(dynamic_cast<BuildStep *>(m_step)->displayName(),
        MaemoGlobal::deviceConfigurationName(m_step->maemoDeployConfig()->deviceConfiguration()));
}

} // namespace Internal
} // namespace RemoteLinux
