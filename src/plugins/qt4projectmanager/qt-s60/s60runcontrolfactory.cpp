/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60runcontrolfactory.h"

#include "codaruncontrol.h"
#include "s60devicerunconfiguration.h"
#include "s60deployconfiguration.h"

#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

S60RunControlFactory::S60RunControlFactory(RunMode mode, const QString &name, QObject *parent) :
    IRunControlFactory(parent), m_mode(mode), m_name(name)
{
}

bool S60RunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    if (mode != m_mode)
        return false;
    S60DeviceRunConfiguration *rc = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
    if (!rc)
        return false;
    S60DeployConfiguration *activeDeployConf = qobject_cast<S60DeployConfiguration *>(rc->target()->activeDeployConfiguration());
    return activeDeployConf != 0;
}

RunControl* S60RunControlFactory::create(RunConfiguration *runConfiguration, RunMode mode)
{
    S60DeviceRunConfiguration *rc = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);

    QTC_ASSERT(rc, return 0);
    QTC_ASSERT(mode == m_mode, return 0);

    S60DeployConfiguration *activeDeployConf = qobject_cast<S60DeployConfiguration *>(rc->target()->activeDeployConfiguration());
    if (!activeDeployConf)
        return 0;
    return new CodaRunControl(rc, mode);
}

QString S60RunControlFactory::displayName() const
{
    return m_name;
}
