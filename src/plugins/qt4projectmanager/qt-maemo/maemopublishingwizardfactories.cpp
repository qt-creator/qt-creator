/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "maemopublishingwizardfactories.h"

#include "maemopublishingwizardfremantlefree.h"
#include "maemotoolchain.h"

#include <projectexplorer/target.h>
#include <qt4projectmanager/qmakestep.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

MaemoPublishingWizardFactoryFremantleFree::MaemoPublishingWizardFactoryFremantleFree(QObject *parent)
    : IPublishingWizardFactory(parent)
{
}

QString MaemoPublishingWizardFactoryFremantleFree::displayName() const
{
    return tr("Publish for \"Fremantle Extras-devel free\" repository");
}

QString MaemoPublishingWizardFactoryFremantleFree::description() const
{
    return tr("This wizard will create a source archive and optionally upload "
              "it to a build server, where the project will be compiled and "
              "packaged and then moved to the \"Extras-devel free\" "
              "repository, from where users can install it onto their N900 "
              "devices. For the upload functionality, an account at "
              "garage.maemo.org is required.");
}

bool MaemoPublishingWizardFactoryFremantleFree::canCreateWizard(const Project *project) const
{
    if (!qobject_cast<const Qt4Project *>(project))
        return false;
    foreach (const Target *const target, project->targets()) {
        if (target->id() != QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
            continue;
        foreach (const BuildConfiguration *const bc, target->buildConfigurations()) {
            const Qt4BuildConfiguration *const qt4Bc
                = qobject_cast<const Qt4BuildConfiguration *>(bc);
            if (!qt4Bc)
                continue;
            const MaemoToolChain * const tc
                = dynamic_cast<MaemoToolChain *>(qt4Bc->toolChain());
            if (!tc)
                continue;
            if (tc->version() == MaemoToolChain::Maemo5)
                return true;
        }
        break;
    }
    return false;
}

QWizard *MaemoPublishingWizardFactoryFremantleFree::createWizard(const Project *project) const
{
    Q_ASSERT(canCreateWizard(project));
    return new MaemoPublishingWizardFremantleFree(project);
}

} // namespace Internal
} // namespace Qt4ProjectManager
