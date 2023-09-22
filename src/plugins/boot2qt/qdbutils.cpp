// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbutils.h"

#include "qdbtr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Qdb::Internal {

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

FilePath findTool(QdbTool tool)
{
    QString filePath = Utils::qtcEnvironmentVariable(overridingEnvironmentVariable(tool));

    if (filePath.isEmpty()) {
        QtcSettings * const settings = Core::ICore::settings();
        settings->beginGroup(settingsGroupKey());
        filePath = settings->value(settingsKey(tool)).toString();
        settings->endGroup();
    }

    if (filePath.isEmpty()) {
        filePath = QCoreApplication::applicationDirPath();
        if (HostOsInfo::isMacHost())
            filePath += "/../../../Tools/b2qt/";
        else
            filePath += "/../../b2qt/";
        filePath = HostOsInfo::withExecutableSuffix(filePath + executableBaseName(tool));
    }

    return FilePath::fromUserInput(filePath);
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
    const QString fullMessage = Tr::tr("Boot2Qt: %1").arg(message);
    if (important)
        Core::MessageManager::writeFlashing(fullMessage);
    else
        Core::MessageManager::writeSilently(fullMessage);
}

Key settingsGroupKey()
{
    return "Boot2Qt";
}

Key settingsKey(QdbTool tool)
{
    switch (tool) {
    case QdbTool::FlashingWizard:
        return "flashingWizardFilePath";
    case QdbTool::Qdb:
        return "qdbFilePath";
    }
    QTC_CHECK(false);
    return {};
}

} // Qdb::Internal
