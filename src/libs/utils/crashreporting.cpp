// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "crashreporting.h"

#include "appinfo.h"
#include "aspects.h"
#include "environment.h"
#include "infobar.h"
#include "layoutbuilder.h"
#include "qtcsettings.h"
#include "utilstr.h"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>

#ifdef ENABLE_SENTRY
#include <QLoggingCategory>
#include <QSysInfo>

#include <sentry.h>

Q_LOGGING_CATEGORY(sentryLog, "qtc.sentry", QtWarningMsg)
#endif

namespace Utils {

const char kCrashReportingSetting[] = "CrashReportingEnabled";

Key crashReportSettingsKey()
{
    return QByteArray(kCrashReportingSetting);
}

bool isCrashReportingAvailable()
{
#ifdef ENABLE_SENTRY
    return !appInfo().crashReports.isEmpty();
#else
    return false;
#endif
}

class SentryInstance
{
public:
    SentryInstance()
    {
#ifdef ENABLE_SENTRY

        sentry_options_t *options = sentry_options_new();
        sentry_options_set_dsn(options, SENTRY_DSN);

#ifdef Q_OS_WIN
        sentry_options_set_database_pathw(
            options, appInfo().crashReports.nativePath().toStdWString().c_str());
#else
        sentry_options_set_database_path(
            options, appInfo().crashReports.nativePath().toUtf8().constData());
#endif
        const QString release = QString(SENTRY_PROJECT) + "@"
                                + QCoreApplication::applicationVersion();
        sentry_options_set_release(options, release.toUtf8().constData());
        sentry_options_set_debug(options, sentryLog().isDebugEnabled() ? 1 : 0);
        int result = sentry_init(options);
        if (result != 0) {
            qCWarning(sentryLog) << "Failed to initialize Sentry:" << result;
            throw std::runtime_error("Failed to initialize Sentry");
        }
        sentry_set_tag("qt.version", qVersion());
        sentry_set_tag("qt.architecture", QSysInfo::buildCpuArchitecture().toUtf8().constData());
        sentry_set_tag("qtcreator.compiler", Utils::compilerString().toUtf8().constData());
        if (!appInfo().revision.isEmpty())
            sentry_set_tag("qtcreator.revision", appInfo().revision.toUtf8().constData());
#endif
    }

    ~SentryInstance()
    {
#ifdef ENABLE_SENTRY
        sentry_close();
#endif
    }
};

void setCrashReportingEnabled(bool enable)
{
    static std::unique_ptr<SentryInstance> theInstance;

    if (enable) {
        try {
            theInstance = std::make_unique<SentryInstance>();
        } catch (...) {
        }
    } else
        theInstance.reset();
}

QString breakpadInformation()
{
    QString backend;
    QString url;

    backend = "Google Breakpad";
    url = "https://chromium.googlesource.com/breakpad/breakpad/+/HEAD/docs/"
          "client_design.md";
    //: %1 = application name, %2 crash backend name (Google Crashpad or Google Breakpad)
    return Tr::tr(
               "%1 uses %2 for collecting crashes and sending them to Sentry "
               "for processing. %2 may capture arbitrary contents from crashed process’ "
               "memory, including user sensitive information, URLs, and whatever other content "
               "users have trusted %1 with. The collected crash reports are however only used "
               "for the sole purpose of fixing bugs.")
               .arg(QGuiApplication::applicationDisplayName(), backend)
           + "<br><br>" + Tr::tr("More information:") + "<br><a href='" + url
           + "'>"
           //: %1 = crash backend name (Google Crashpad or Google Breakpad)
           + Tr::tr("%1 Overview").arg(backend)
           + "</a>"
             "<br><a href='https://sentry.io/security/'>"
           + Tr::tr("%1 security policy").arg("Sentry.io") + "</a>";
}

} // namespace Utils
