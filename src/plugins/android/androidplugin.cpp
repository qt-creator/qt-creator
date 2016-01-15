/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidplugin.h"

#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androiddeployqtstep.h"
#include "androiddevice.h"
#include "androiddevicefactory.h"
#include "androidmanager.h"
#include "androidrunfactories.h"
#include "androidsettingspage.h"
#include "androidtoolchain.h"
#include "androidqtversionfactory.h"
#include "androiddeployconfiguration.h"
#include "androidgdbserverkitinformation.h"
#include "androidmanifesteditorfactory.h"
#include "androidpotentialkit.h"
#include "javacompletionassistprovider.h"
#include "javaeditor.h"
#ifdef HAVE_QBS
#  include "androidqbspropertyprovider.h"
#endif

#include <coreplugin/icore.h>
#include <projectexplorer/kitmanager.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QtPlugin>

#include <projectexplorer/devicesupport/devicemanager.h>

namespace Android {

AndroidPlugin::AndroidPlugin()
{ }

bool AndroidPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    new AndroidConfigurations(this);

    addAutoReleasedObject(new Internal::AndroidRunControlFactory);
    addAutoReleasedObject(new Internal::AndroidDeployQtStepFactory);
    addAutoReleasedObject(new Internal::AndroidSettingsPage);
    addAutoReleasedObject(new Internal::AndroidQtVersionFactory);
    addAutoReleasedObject(new Internal::AndroidToolChainFactory);
    addAutoReleasedObject(new Internal::AndroidDeployConfigurationFactory);
    addAutoReleasedObject(new Internal::AndroidDeviceFactory);
    addAutoReleasedObject(new Internal::AndroidPotentialKit);
    addAutoReleasedObject(new Internal::JavaEditorFactory);
    ProjectExplorer::KitManager::registerKitInformation(new Internal::AndroidGdbServerKitInformation);

    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/android/Android.mimetypes.xml"));

    addAutoReleasedObject(new Internal::AndroidManifestEditorFactory);

    connect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsLoaded()),
            this, SLOT(kitsRestored()));

    connect(ProjectExplorer::DeviceManager::instance(), SIGNAL(devicesLoaded()),
            this, SLOT(updateDevice()));
    return true;
}

void AndroidPlugin::kitsRestored()
{
    AndroidConfigurations::updateAutomaticKitList();
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            AndroidConfigurations::instance(), SLOT(updateAutomaticKitList()));
    disconnect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsChanged()),
               this, SLOT(kitsRestored()));
}

void AndroidPlugin::updateDevice()
{
    AndroidConfigurations::updateAndroidDevice();
}

} // namespace Android
