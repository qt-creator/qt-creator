/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "project/nimrunconfigurationfactory.h"

#include "nimconstants.h"
#include "project/nimproject.h"
#include "project/nimrunconfiguration.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/target.h>

#include <memory>

using namespace ProjectExplorer;

namespace Nim {

NimRunConfigurationFactory::NimRunConfigurationFactory()
{
}

QList<Core::Id> NimRunConfigurationFactory::availableCreationIds(Target *parent,
                                                                 IRunConfigurationFactory::CreationMode mode) const
{
    Q_UNUSED(mode);
    if (canHandle(parent))
        return { Constants::C_NIMRUNCONFIGURATION_ID };
    return {};
}

QString NimRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    QString result = id.toString() + QStringLiteral("-TempRunConf");
    return result;
}

bool NimRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    Q_UNUSED(id);
    return canHandle(parent);
}

bool NimRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    Q_UNUSED(parent);
    Q_UNUSED(map);
    return canHandle(parent);
}

bool NimRunConfigurationFactory::canClone(Target *parent, RunConfiguration *product) const
{
    QTC_ASSERT(parent, return false);
    QTC_ASSERT(product, return false);
    return true;
}

RunConfiguration *NimRunConfigurationFactory::clone(Target *parent, RunConfiguration *product)
{
    QTC_ASSERT(parent, return nullptr);
    QTC_ASSERT(product, return nullptr);
    std::unique_ptr<NimRunConfiguration> result(new NimRunConfiguration(parent, Constants::C_NIMRUNCONFIGURATION_ID));
    return result->fromMap(product->toMap()) ? result.release() : nullptr;
}

bool NimRunConfigurationFactory::canHandle(Target *parent) const
{
    Q_UNUSED(parent);
    return qobject_cast<NimProject *>(parent->project());
}

RunConfiguration *NimRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    Q_UNUSED(id);
    auto result = new NimRunConfiguration(parent, id);
    return result;
}

RunConfiguration *NimRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    Q_UNUSED(map);
    auto result = new NimRunConfiguration(parent, idFromMap(map));
    result->fromMap(map);
    return result;
}

}
