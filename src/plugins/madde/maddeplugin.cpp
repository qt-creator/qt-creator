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

#include "maddeplugin.h"

#include "debianmanager.h"
#include "maddedeviceconfigurationfactory.h"
#include "maemoconstants.h"
#include "maemodeploystepfactory.h"
#include "maemopackagecreationfactory.h"
#include "maemopublishingwizardfactories.h"
#include "maemoqemumanager.h"
#include "maemorunfactories.h"
#include "maemosettingspages.h"
#include "qt4maemodeployconfiguration.h"
#include "rpmmanager.h"
#include "maemoqtversionfactory.h"

#include <QtPlugin>

namespace Madde {
namespace Internal {

MaddePlugin::MaddePlugin()
{
}

MaddePlugin::~MaddePlugin()
{
}

bool MaddePlugin::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error_message)

    MaemoQemuManager::instance(this);

    addAutoReleasedObject(new MaemoRunControlFactory);
    addAutoReleasedObject(new MaemoRunConfigurationFactory);
    addAutoReleasedObject(new Qt4MaemoDeployConfigurationFactory);
    addAutoReleasedObject(new MaemoPackageCreationFactory);
    addAutoReleasedObject(new MaemoDeployStepFactory);
    addAutoReleasedObject(new MaemoQemuSettingsPage);
    addAutoReleasedObject(new MaemoPublishingWizardFactoryFremantleFree);
    addAutoReleasedObject(new MaemoQtVersionFactory);
    addAutoReleasedObject(new MaddeDeviceConfigurationFactory);

    new DebianManager(this);
    new RpmManager(this);

    return true;
}

void MaddePlugin::extensionsInitialized()
{
}

} // namespace Internal
} // namespace Madde

Q_EXPORT_PLUGIN(Madde::Internal::MaddePlugin)
