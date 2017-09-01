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

#include "qmlprojectrunconfigurationfactory.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlproject.h"
#include "qmlprojectrunconfiguration.h"

#include <projectexplorer/kitinformation.h>
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

QList<Core::Id> QmlProjectRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
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

QString QmlProjectRunConfigurationFactory::displayNameForId(Core::Id id) const
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

ProjectExplorer::RunConfiguration *QmlProjectRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, Core::Id id)
{
    return createHelper<QmlProjectRunConfiguration>(parent, id);
}

bool QmlProjectRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return parent && canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::RunConfiguration *QmlProjectRunConfigurationFactory::doRestore(ProjectExplorer::Target *parent,
                                                                                const QVariantMap &map)
{
    return createHelper<QmlProjectRunConfiguration>(parent, ProjectExplorer::idFromMap(map));
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
    return cloneHelper<QmlProjectRunConfiguration>(parent, source);
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

