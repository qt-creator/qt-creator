/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <projectexplorer/kitmanager.h>
#include <qtsupport/qtversionmanager.h>

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

    // AndroidManifest.xml editor
    Core::MimeGlobPattern androidManifestGlobPattern(QLatin1String("AndroidManifest.xml"), Core::MimeGlobPattern::MaxWeight);
    Core::MimeType androidManifestMimeType;
    androidManifestMimeType.setType(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));
    androidManifestMimeType.setComment(tr("Android Manifest file"));
    androidManifestMimeType.setGlobPatterns(QList<Core::MimeGlobPattern>() << androidManifestGlobPattern);
    androidManifestMimeType.setSubClassesOf(QStringList() << QLatin1String("application/xml"));

    if (!Core::MimeDatabase::addMimeType(androidManifestMimeType)) {
        *errorMessage = tr("Could not add mime-type for AndroidManifest.xml editor.");
        return false;
    }
    addAutoReleasedObject(new Internal::AndroidManifestEditorFactory);

    if (!Core::MimeDatabase::addMimeTypes(QLatin1String(":android/Java.mimetypes.xml"), errorMessage))
        return false;

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
