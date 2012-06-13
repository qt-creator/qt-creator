/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunfactories.h"

#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidtarget.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggerconstants.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>


namespace Android {
namespace Internal {

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace {

QString pathFromId(const Core::Id id)
{
    QString pathStr = QString::fromUtf8(id.name());
    const QString prefix = QLatin1String(ANDROID_RC_ID_PREFIX);
    if (!pathStr.startsWith(prefix))
        return QString();
    return pathStr.mid(prefix.size());
}

} // namespace

AndroidRunConfigurationFactory::AndroidRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

AndroidRunConfigurationFactory::~AndroidRunConfigurationFactory()
{
}

bool AndroidRunConfigurationFactory::canCreate(Target *parent,
    const Core::Id/*id*/) const
{
    AndroidTarget *target = qobject_cast<AndroidTarget *>(parent);
    if (!target
            || target->id() != Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID)) {
        return false;
    }
    return true;
}

bool AndroidRunConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    Q_UNUSED(parent)
    Q_UNUSED(map)
    if (!qobject_cast<AndroidTarget *>(parent))
        return false;
    QString id = QString::fromUtf8(ProjectExplorer::idFromMap(map).name());
    return id.startsWith(QLatin1String(ANDROID_RC_ID));
}

bool AndroidRunConfigurationFactory::canClone(Target *parent,
    RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QList<Core::Id> AndroidRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (AndroidTarget *t = qobject_cast<AndroidTarget *>(parent)) {
        if (t->id() == Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID)) {
            QList<Qt4ProFileNode *> nodes = t->qt4Project()->allProFiles();
            foreach (Qt4ProFileNode *node, nodes)
                if (node->projectType() == ApplicationTemplate || node->projectType() == LibraryTemplate)
                    ids << Core::Id(node->targetInformation().target);
        }
    }
    return ids;
}

QString AndroidRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName();
}

RunConfiguration *AndroidRunConfigurationFactory::create(Target *parent,
    const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    AndroidTarget *pqt4parent = static_cast<AndroidTarget *>(parent);
    return new AndroidRunConfiguration(pqt4parent, pathFromId(id));

}

RunConfiguration *AndroidRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AndroidTarget *target = static_cast<AndroidTarget *>(parent);
    AndroidRunConfiguration *rc = new AndroidRunConfiguration(target, QString());
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

RunConfiguration *AndroidRunConfigurationFactory::clone(Target *parent,
    RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    AndroidRunConfiguration *old = static_cast<AndroidRunConfiguration *>(source);
    return new AndroidRunConfiguration(static_cast<AndroidTarget *>(parent), old);
}

// #pragma mark -- AndroidRunControlFactory

AndroidRunControlFactory::AndroidRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

AndroidRunControlFactory::~AndroidRunControlFactory()
{
}

bool AndroidRunControlFactory::canRun(RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const
{
    if (mode != NormalRunMode && mode != DebugRunMode)
        return false;
    return qobject_cast<AndroidRunConfiguration *>(runConfiguration);
}

RunControl *AndroidRunControlFactory::create(RunConfiguration *runConfig,
                                        ProjectExplorer::RunMode mode)
{
    Q_ASSERT(canRun(runConfig, mode));
    AndroidRunConfiguration *rc = qobject_cast<AndroidRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == NormalRunMode)
        return new AndroidRunControl(rc);
    else
        return AndroidDebugSupport::createDebugRunControl(rc);
}

QString AndroidRunControlFactory::displayName() const
{
    return tr("Run on Android device/emulator");
}

} // namespace Internal
} // namespace Qt4ProjectManager
