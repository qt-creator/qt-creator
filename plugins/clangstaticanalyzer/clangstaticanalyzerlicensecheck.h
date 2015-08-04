/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
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

#ifndef CLANGSTATICANALYZERLICENSECHECK_H
#define CLANGSTATICANALYZERLICENSECHECK_H

#ifdef LICENSECHECKER
#include <extensionsystem/pluginmanager.h>
#include <licensechecker/licensecheckerplugin.h>
#endif

inline bool enterpriseFeaturesAvailable()
{
#ifdef LICENSECHECKER
    LicenseChecker::LicenseCheckerPlugin *licenseChecker
            = ExtensionSystem::PluginManager::getObject<LicenseChecker::LicenseCheckerPlugin>();

    if (licenseChecker && licenseChecker->hasValidLicense()) {
        if (licenseChecker->enterpriseFeatures()) {
            return true;
        } else {
            qWarning() << "License does not cover enterprise features, "
                          "disabling Clang Static Analyzer";
        }
    } else {
        qWarning() << "Invalid license, disabling Clang Static Analyzer";
    }
    return false;
#else // LICENSECHECKER
    return true;
#endif
}

#endif // Include guard.
