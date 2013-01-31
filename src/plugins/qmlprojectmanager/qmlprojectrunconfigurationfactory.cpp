/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmlprojectmanagerconstants.h"
#include "qmlproject.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojectrunconfigurationfactory.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>

namespace QmlProjectManager {
namespace Internal {

QmlProjectRunConfigurationFactory::QmlProjectRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("QmlProjectRunConfigurationFactory"));
}

QmlProjectRunConfigurationFactory::~QmlProjectRunConfigurationFactory()
{
}

QList<Core::Id> QmlProjectRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    QtSupport::BaseQtVersion *version
            = QtSupport::QtKitInformation::qtVersion(parent->kit());

    // First id will be the default run configuration
    QList<Core::Id> list;
    if (version && version->qtVersion() >= QtSupport::QtVersionNumber(5, 0, 0)) {
        QmlProject *project = static_cast<QmlProject*>(parent->project());
        switch (project->defaultImport()) {
        case QmlProject::QtQuick1Import:
            list << Core::Id(Constants::QML_VIEWER_RC_ID);
            break;
        case QmlProject::QtQuick2Import:
            list << Core::Id(Constants::QML_SCENE_RC_ID);
            break;
        case QmlProject::UnknownImport:
        default:
            list << Core::Id(Constants::QML_SCENE_RC_ID);
            list << Core::Id(Constants::QML_VIEWER_RC_ID);
            break;
        }
    } else {
        list << Core::Id(Constants::QML_VIEWER_RC_ID);
    }

    return list;
}

QString QmlProjectRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Constants::QML_VIEWER_RC_ID)
        return tr("QML Viewer");
    if (id == Constants::QML_SCENE_RC_ID)
        return tr("QML Scene");
    return QString();
}

bool QmlProjectRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                                  const Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    if (id == Constants::QML_VIEWER_RC_ID)
        return true;

    if (id == Constants::QML_SCENE_RC_ID) {
        // only support qmlscene if it's Qt5
        QtSupport::BaseQtVersion *version
                = QtSupport::QtKitInformation::qtVersion(parent->kit());
        return version && version->qtVersion() >= QtSupport::QtVersionNumber(5, 0, 0);
    }
    return false;
}

ProjectExplorer::RunConfiguration *QmlProjectRunConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new QmlProjectRunConfiguration(parent, id);
}

bool QmlProjectRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return parent && canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::RunConfiguration *QmlProjectRunConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Core::Id id = ProjectExplorer::idFromMap(map);
    QmlProjectRunConfiguration *rc = new QmlProjectRunConfiguration(parent, id);
    if (rc->fromMap(map))
        return rc;
    delete rc;
    return 0;
}

bool QmlProjectRunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *QmlProjectRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                     ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new QmlProjectRunConfiguration(parent, qobject_cast<QmlProjectRunConfiguration *>(source));
}

bool QmlProjectRunConfigurationFactory::canHandle(ProjectExplorer::Target *parent) const
{
    if (!parent->project()->supportsKit(parent->kit()))
        return false;
    if (!qobject_cast<QmlProject *>(parent->project()))
        return false;
    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    return deviceType == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

} // namespace Internal
} // namespace QmlProjectManager

