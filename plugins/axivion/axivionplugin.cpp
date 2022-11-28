/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Axivion Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "axivionplugin.h"

#include "axivionoutputpane.h"
#include "axivionsettings.h"
#include "axivionsettingspage.h"


#ifdef LICENSECHECKER
#  include <licensechecker/licensecheckerplugin.h>
#endif

#include <extensionsystem/pluginmanager.h>

namespace Axivion::Internal {

class AxivionPluginPrivate
{
public:
    AxivionSettings axivionSettings;
    AxivionSettingsPage axivionSettingsPage{&axivionSettings};
    AxivionOutputPane axivionOutputPane;
};

AxivionPlugin::~AxivionPlugin()
{
    delete d;
}

bool AxivionPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

#ifdef LICENSECHECKER
    LicenseChecker::LicenseCheckerPlugin *licenseChecker
            = ExtensionSystem::PluginManager::getObject<LicenseChecker::LicenseCheckerPlugin>();

    if (!licenseChecker || !licenseChecker->hasValidLicense() || !licenseChecker->enterpriseFeatures())
        return true;
#endif // LICENSECHECKER

    d = new AxivionPluginPrivate;
    return true;
}

} // Axivion::Internal
