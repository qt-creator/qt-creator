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

    return true;
}

void MaddePlugin::extensionsInitialized()
{
}

} // namespace Internal
} // namespace Madde

Q_EXPORT_PLUGIN(Madde::Internal::MaddePlugin)
