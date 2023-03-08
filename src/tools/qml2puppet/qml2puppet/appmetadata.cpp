// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "appmetadata.h"

#include <app/app_version.h>

namespace QDSMeta::AppInfo {

void printAppInfo()
{
    qInfo() << Qt::endl
            << "<< QDS Meta Info >>" << Qt::endl
            << "App Info" << Qt::endl
            << " - Name    :" << Core::Constants::IDE_ID << Qt::endl
            << " - Version :" << Core::Constants::IDE_VERSION_DISPLAY << Qt::endl
            << " - Author  :" << Core::Constants::IDE_AUTHOR << Qt::endl
            << " - Year    :" << Core::Constants::IDE_YEAR << Qt::endl
            << " - App     :" << QCoreApplication::applicationName() << Qt::endl
            << "Build Info " << Qt::endl
            << " - Date       :" << __DATE__ << Qt::endl
            << " - Commit     :" << QStringLiteral(QDS_STRINGIFY(IDE_REVISION_STR)) << Qt::endl
            << " - Qt Version :" << QT_VERSION_STR << Qt::endl
            << "Compiler Info " << Qt::endl
#if defined(__GNUC__)
            << " - GCC       :" << __GNUC__ << Qt::endl
            << " - GCC Minor :" << __GNUC_MINOR__ << Qt::endl
            << " - GCC Patch :" << __GNUC_PATCHLEVEL__ << Qt::endl
#endif
#if defined(_MSC_VER)
            << " - MSC Short :" << _MSC_VER << Qt::endl
            << " - MSC Full  :" << _MSC_FULL_VER << Qt::endl
#endif
#if defined(__clang__)
            << " - clang maj   :" << __clang_major__ << Qt::endl
            << " - clang min   :" << __clang_minor__ << Qt::endl
            << " - clang patch :" << __clang_patchlevel__ << Qt::endl
#endif
            << "<< End Of QDS Meta Info >>" << Qt::endl;
    exit(0);
}

void registerAppInfo(const QString &appName)
{
    QCoreApplication::setOrganizationName(Core::Constants::IDE_AUTHOR);
    QCoreApplication::setOrganizationDomain("qt-project.org");
    QCoreApplication::setApplicationName(appName);
    QCoreApplication::setApplicationVersion(Core::Constants::IDE_VERSION_LONG);
}

} // namespace QDSMeta::AppInfo
