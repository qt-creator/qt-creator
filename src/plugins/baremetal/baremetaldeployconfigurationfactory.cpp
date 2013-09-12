/****************************************************************************
**
** Copyright (C) 2013 Tim Sander <tim@krieglstein.org>
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

#include <QCoreApplication>

#include "baremetaldeployconfigurationfactory.h"
#include "baremetalconstants.h"
#include "baremetaldeployconfiguration.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {
namespace {
QString genericBareMetalDisplayName() {
    return QCoreApplication::translate("BareMetal","Run on gdbserver/Hardware Debugger");
}
} // anonymous namespace


BareMetalDeployConfigurationFactory::BareMetalDeployConfigurationFactory(QObject *parent) :
    DeployConfigurationFactory(parent)
{
    setObjectName(QLatin1String("BareMetalDeployConfiguration"));
}

QList<Core::Id> BareMetalDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (!parent->project()->supportsKit(parent->kit()))
        return ids;
    ToolChain *tc
            = ToolChainKitInformation::toolChain(parent->kit());
    if (!tc || tc->targetAbi().os() != Abi::LinuxOS) //FIXME
        return ids;
    const Core::Id devType = DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if (devType == Constants::BareMetalOsType)
        ids << genericDeployConfigurationId();
    return ids;
}

QString BareMetalDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == genericDeployConfigurationId())
        return genericBareMetalDisplayName();
    return QString();
}

bool BareMetalDeployConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *BareMetalDeployConfigurationFactory::create(Target *parent, const Core::Id id)
{
    QTC_ASSERT(canCreate(parent, id), return 0);
    DeployConfiguration * const dc = new BareMetalDeployConfiguration(parent,id,genericBareMetalDisplayName());
//FIXME    dc->stepList()->insertStep(0, Step);
    return dc;
}

bool BareMetalDeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent,idFromMap(map));
}

DeployConfiguration *BareMetalDeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent,map))
        return 0;
    Core::Id id = idFromMap(map);
    BareMetalDeployConfiguration * const dc = new BareMetalDeployConfiguration(parent, id, genericBareMetalDisplayName());
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

DeployConfiguration *BareMetalDeployConfigurationFactory::clone(Target *parent, DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new BareMetalDeployConfiguration(parent,
                                            qobject_cast<BareMetalDeployConfiguration*>(product));
}

Core::Id BareMetalDeployConfigurationFactory::genericDeployConfigurationId()
{
    return Core::Id("DeployToBareMetal");
}

} // namespace Internal
} // namespace BareMetal
