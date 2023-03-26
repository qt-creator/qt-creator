// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QCommandLineParser>
#include <QLoggingCategory>

#include <app/app_version.h>

// Common functions can be used in all QDS apps
namespace QDSMeta {

namespace Logging {
inline Q_LOGGING_CATEGORY(deprecated, "qt.tools.qds.deprecated");
inline Q_LOGGING_CATEGORY(verbose1, "qt.tools.qds.verbose1");
inline Q_LOGGING_CATEGORY(verbose2, "qt.tools.qds.verbose2");

inline void registerMessageHandler()
{
    qInstallMessageHandler(
        [](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
            auto tPrinter = [&](const QString &msgPrefix) {
                fprintf(stderr,
                        "%s: %s (%s:%u, %s)\n",
                        msgPrefix.toLocal8Bit().constData(),
                        msg.toLocal8Bit().constData(),
                        context.file,
                        context.line,
                        context.function);
            };

            if (type == QtDebugMsg)
                tPrinter("Debug");
            else if (type == QtInfoMsg)
                tPrinter("Info");
            else if (type == QtWarningMsg)
                tPrinter("Warning");
            else if (type == QtCriticalMsg)
                tPrinter("Critical");
            else if (type == QtFatalMsg) {
                tPrinter("Fatal");
                abort();
            }
        });
}
} // namespace Logging

namespace AppInfo {

#define STRINGIFY_INTERNAL(x) #x
#define QDS_STRINGIFY(x) STRINGIFY_INTERNAL(x)

inline void printAppInfo()
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
            << " - Commit     :" << QStringLiteral(QDS_STRINGIFY(GIT_SHA)) << Qt::endl
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

inline void registerAppInfo(const QString &appName)
{
    QCoreApplication::setOrganizationName(Core::Constants::IDE_AUTHOR);
    QCoreApplication::setOrganizationDomain("qt-project.org");
    QCoreApplication::setApplicationName(appName);
    QCoreApplication::setApplicationVersion(Core::Constants::IDE_VERSION_LONG);
}

} // namespace AppInfo
} // namespace QDSMeta
