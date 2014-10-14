/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baremetaldeploystepfactory.h"

#include "baremetaldeployconfiguration.h"
#include "baremetalgdbcommandsdeploystep.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {
BareMetalDeployStepFactory::BareMetalDeployStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

QList<Core::Id> BareMetalDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    QList<Core::Id> ids;
    if (!qobject_cast<BareMetalDeployConfiguration *>(parent->parent()))
        return ids;
    ids << BareMetalGdbCommandsDeployStep::stepId();
    return ids;
}

QString BareMetalDeployStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == BareMetalGdbCommandsDeployStep::stepId())
        return BareMetalGdbCommandsDeployStep::displayName();
    return QString();
}

bool BareMetalDeployStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

BuildStep *BareMetalDeployStepFactory::create(BuildStepList *parent, const Core::Id id)
{
    QTC_ASSERT(canCreate(parent, id), return 0);
    if (id == BareMetalGdbCommandsDeployStep::stepId())
        return new BareMetalGdbCommandsDeployStep(parent, id);
    return 0;
}

bool BareMetalDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *BareMetalDeployStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return 0);
    BuildStep * const step = create(parent, idFromMap(map));
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool BareMetalDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *BareMetalDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    if (BareMetalGdbCommandsDeployStep * const other = qobject_cast<BareMetalGdbCommandsDeployStep *>(product))
        return new BareMetalGdbCommandsDeployStep(parent, other);
    return 0;
}

} // namespace Internal
} // namepsace BareMetal
