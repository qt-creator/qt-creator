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

#include "androidplugin.h"

#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androiddeployqtstep.h"
#include "androiddevice.h"
#include "androiddevicefactory.h"
#include "androidmanager.h"
#include "androidpackageinstallationfactory.h"
#include "androidrunfactories.h"
#include "androidsettingspage.h"
#include "androidtoolchain.h"
#include "androidqtversionfactory.h"
#include "androiddeployconfiguration.h"
#include "androidgdbserverkitinformation.h"
#include "androidmanifesteditorfactory.h"
#include "androidpotentialkit.h"
#include "javaeditorfactory.h"
#include "javacompletionassistprovider.h"
#include "javafilewizard.h"
#include "qmakeandroidsupport.h"
#include "qmakeandroidrunfactories.h"
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

    new Internal::AndroidConfigurations(this);

    addAutoReleasedObject(new Internal::AndroidRunControlFactory);
    addAutoReleasedObject(new Internal::QmakeAndroidRunConfigurationFactory);
    addAutoReleasedObject(new Internal::AndroidPackageInstallationFactory);
    addAutoReleasedObject(new Internal::AndroidDeployQtStepFactory);
    addAutoReleasedObject(new Internal::AndroidSettingsPage);
    addAutoReleasedObject(new Internal::AndroidQtVersionFactory);
    addAutoReleasedObject(new Internal::AndroidToolChainFactory);
    addAutoReleasedObject(new Internal::AndroidDeployConfigurationFactory);
    addAutoReleasedObject(new Internal::AndroidDeviceFactory);
    addAutoReleasedObject(new Internal::AndroidPotentialKit);
    addAutoReleasedObject(new Internal::JavaEditorFactory);
    addAutoReleasedObject(new Internal::JavaCompletionAssistProvider);
    addAutoReleasedObject(new Internal::JavaFileWizard);
    addAutoReleasedObject(new Internal::QmakeAndroidSupport);
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
    Internal::AndroidConfigurations::updateAutomaticKitList();
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            Internal::AndroidConfigurations::instance(), SLOT(updateAutomaticKitList()));
    disconnect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsChanged()),
               this, SLOT(kitsRestored()));
}

void AndroidPlugin::updateDevice()
{
    Internal::AndroidConfigurations::updateAndroidDevice();
}

} // namespace Android

Q_EXPORT_PLUGIN(Android::AndroidPlugin)
