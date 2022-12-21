// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDir>

namespace Qdb {
namespace Internal {

static QString executableBaseName(QdbTool tool)
{
    switch (tool) {
    case QdbTool::FlashingWizard:
        return QLatin1String("b2qt-flashing-wizard");
    case QdbTool::Qdb:
        return QLatin1String("qdb");
    }
    QTC_CHECK(false);
    return QString();
}

Utils::FilePath findTool(QdbTool tool)
{
    QString filePath = Utils::qtcEnvironmentVariable(overridingEnvironmentVariable(tool));

    if (filePath.isEmpty()) {
        QSettings * const settings = Core::ICore::settings();
        settings->beginGroup(settingsGroupKey());
        filePath = settings->value(settingsKey(tool)).toString();
        settings->endGroup();
    }

    if (filePath.isEmpty()) {
        filePath = Utils::HostOsInfo::withExecutableSuffix(
                    QCoreApplication::applicationDirPath()
#ifdef Q_OS_MACOS
                    + QLatin1String("/../../../Tools/b2qt/")
#else
                    + QLatin1String("/../../b2qt/")
#endif
                    + executableBaseName(tool));
    }

    return Utils::FilePath::fromString(QDir::cleanPath(filePath));
}

QString overridingEnvironmentVariable(QdbTool tool)
{
    switch (tool) {
    case QdbTool::FlashingWizard:
        return "BOOT2QT_FLASHWIZARD_FILEPATH";
    case QdbTool::Qdb:
        return "BOOT2QT_QDB_FILEPATH";
    }
    QTC_ASSERT(false, return {});
}

void showMessage(const QString &message, bool important)
{
    const QString fullMessage = QCoreApplication::translate("Boot2Qt", "Boot2Qt: %1").arg(message);
    if (important)
        Core::MessageManager::writeFlashing(fullMessage);
    else
        Core::MessageManager::writeSilently(fullMessage);
}

QString settingsGroupKey()
{
    return QLatin1String("Boot2Qt");
}

QString settingsKey(QdbTool tool)
{
    switch (tool) {
    case QdbTool::FlashingWizard:
        return QLatin1String("flashingWizardFilePath");
    case QdbTool::Qdb:
        return QLatin1String("qdbFilePath");
    }
    QTC_ASSERT(false, return QString());
}

} // namespace Internal
} // namespace Qdb
