// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltraceviewerinit.h"

#include "qmltraceviewersettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/manhattanstyle.h>

#include <utils/appinfo.h>
#include <utils/hostosinfo.h>
#include <utils/stylehelper.h>
#include <utils/temporarydirectory.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <app/app_version.h>

#include <QApplication>
#include <QStyleHints>
#include <QSettings>
#include <QStringConverter>

using namespace Utils;
using namespace StyleHelper;

using namespace Qt::StringLiterals;

namespace QmlTraceViewer {

static QString relativePathFromAppDirToShare()
{
    switch (HostOsInfo::hostOs()) {
    case OsTypeMac:
        // TODO
        return "../Resources";
    case OsTypeWindows:
        return "../share/qtcreator/";
    case OsTypeLinux:
        default: return "../../share/qtcreator/";
    };
}

static void initAppInfo()
{
    using namespace Core;

    AppInfo info;
    info.author = Constants::IDE_AUTHOR;
    info.copyright = Constants::IDE_COPYRIGHT;
    info.displayVersion = Constants::IDE_VERSION_DISPLAY;
    info.id = QApplication::applicationName().toLower();
    info.revision = Constants::IDE_REVISION_STR;
    info.revisionUrl = Constants::IDE_REVISION_URL;
    const FilePath appDirPath = FilePath::fromUserInput(QApplication::applicationDirPath());
    info.resources = (appDirPath / relativePathFromAppDirToShare()).cleanPath();
#ifdef QTC_SHOW_BUILD_DATE
    const auto dateTime = QLatin1String(__DATE__ " " __TIME__);
    info.buildTime = QDateTime::fromString(dateTime, "MMM d yyyy hh:mm:ss");
    if (!info.buildTime.isValid()) // single digit is prefixed with space
        info.buildTime = QDateTime::fromString(dateTime, "MMM  d yyyy hh:mm:ss");
    QTC_CHECK(info.buildTime.isValid());
#endif
    Utils::Internal::setAppInfo(info);
}

static void initTemporaryDirectory()
{
    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/"
                                                    + QApplication::applicationName().toLower()
                                                    + "-XXXXXX");
}

static void initTheme()
{
    const Qt::ColorScheme systemColorScheme = QApplication::styleHints()->colorScheme();
    const QLatin1String scheme(systemColorScheme == Qt::ColorScheme::Dark ? "dark" : "light");
    const QString fileName = "%1-2024.creatortheme"_L1.arg(scheme);
    const FilePath themeFile = Core::ICore::resourcePath("themes") / fileName;
    QSettings settings(themeFile.toFSPathString(), QSettings::IniFormat);
    static Theme theme("");
    theme.readSettings(settings);
    setCreatorTheme(&theme);
}

static void initSettings()
{
    settings().readSettings();
}

static void deinitSettings()
{
    settings().writeSettings();
}

static void initStyle()
{
    const QString baseName = QApplication::style()->objectName();
    QApplication::setStyle(new ManhattanStyle(baseName));
    setBaseColor(QColor(DEFAULT_BASE_COLOR));
    setToolbarStyle(ToolbarStyle::Relaxed);
}

void init()
{
    initAppInfo();
    initTemporaryDirectory();
    initTheme();
    initSettings();
    initStyle();
}

void deinit()
{
    deinitSettings();
}

} // namespace QmlTraceViewer
