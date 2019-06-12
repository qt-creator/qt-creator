/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qdbutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/hostosinfo.h>
#include <utils/fileutils.h>
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
    QString filePath = QString::fromLocal8Bit(qgetenv(overridingEnvironmentVariable(tool)));

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

const char *overridingEnvironmentVariable(QdbTool tool)
{
    switch (tool) {
    case QdbTool::FlashingWizard:
        return "BOOT2QT_FLASHWIZARD_FILEPATH";
    case QdbTool::Qdb:
        return "BOOT2QT_QDB_FILEPATH";
    }
    QTC_ASSERT(false, return "");
}

void showMessage(const QString &message, bool important)
{
    const QString fullMessage = QCoreApplication::translate("Boot2Qt", "Boot2Qt: %1").arg(message);
    const Core::MessageManager::PrintToOutputPaneFlags flags = important
            ? Core::MessageManager::ModeSwitch : Core::MessageManager::Silent;
    Core::MessageManager::write(fullMessage, flags);
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
