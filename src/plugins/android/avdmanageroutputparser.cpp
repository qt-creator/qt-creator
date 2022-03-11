/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "avdmanageroutputparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/optional.h>
#include <utils/qtcassert.h>
#include <utils/variant.h>

#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSettings>

namespace {
Q_LOGGING_CATEGORY(avdOutputParserLog, "qtc.android.avdOutputParser", QtWarningMsg)
}

// Avd list keys to parse avd
const char avdInfoNameKey[] = "Name:";
const char avdInfoPathKey[] = "Path:";
const char avdInfoAbiKey[] = "abi.type";
const char avdInfoTargetKey[] = "target";
const char avdInfoErrorKey[] = "Error:";
const char avdInfoSdcardKey[] = "Sdcard";
const char avdInfoTargetTypeKey[] = "Target";
const char avdInfoDeviceKey[] = "Device";
const char avdInfoSkinKey[] = "Skin";

namespace Android {
namespace Internal {

/*!
    Parses the \a line for a [spaces]key[spaces]value[spaces] pattern and returns
    \c true if the key is found, \c false otherwise. The value is copied into \a value.
 */
static bool valueForKey(QString key, const QString &line, QString *value = nullptr)
{
    auto trimmedInput = line.trimmed();
    if (trimmedInput.startsWith(key)) {
        if (value)
            *value = trimmedInput.section(key, 1, 1).trimmed();
        return true;
    }
    return false;
}

static Utils::optional<AndroidDeviceInfo> parseAvd(const QStringList &deviceInfo)
{
    AndroidDeviceInfo avd;
    for (const QString &line : deviceInfo) {
        QString value;
        if (valueForKey(avdInfoErrorKey, line)) {
            qCDebug(avdOutputParserLog) << "Avd Parsing: Skip avd device. Error key found:" << line;
            return {};
        } else if (valueForKey(avdInfoNameKey, line, &value)) {
            avd.avdname = value;
        } else if (valueForKey(avdInfoPathKey, line, &value)) {
            const Utils::FilePath avdPath = Utils::FilePath::fromUserInput(value);
            if (avdPath.exists()) {
                // Get ABI.
                const Utils::FilePath configFile = avdPath.pathAppended("config.ini");
                QSettings config(configFile.toString(), QSettings::IniFormat);
                value = config.value(avdInfoAbiKey).toString();
                if (!value.isEmpty())
                    avd.cpuAbi << value;
                else
                    qCDebug(avdOutputParserLog) << "Avd Parsing: Cannot find ABI:" << configFile;

                // Get Target
                const QString avdInfoFileName = avd.avdname + ".ini";
                const Utils::FilePath avdInfoFile = avdPath.parentDir().pathAppended(
                    avdInfoFileName);
                QSettings avdInfo(avdInfoFile.toString(), QSettings::IniFormat);
                value = avdInfo.value(avdInfoTargetKey).toString();
                if (!value.isEmpty())
                    avd.sdk = platformNameToApiLevel(value);
                else
                    qCDebug(avdOutputParserLog)
                        << "Avd Parsing: Cannot find sdk API:" << avdInfoFile.toString();
            }
        } else if (valueForKey(avdInfoDeviceKey, line, &value)) {
            avd.avdDevice = value.remove(0, 2);
        } else if (valueForKey(avdInfoTargetTypeKey, line, &value)) {
            avd.avdTarget = value.remove(0, 2);
        } else if (valueForKey(avdInfoSkinKey, line, &value)) {
            avd.avdSkin = value.remove(0, 2);
        } else if (valueForKey(avdInfoSdcardKey, line, &value)) {
            avd.avdSdcardSize = value.remove(0, 2);
        }
    }
    if (avd != AndroidDeviceInfo())
        return avd;
    return {};
}

AndroidDeviceInfoList parseAvdList(const QString &output, QStringList *avdErrorPaths)
{
    QTC_CHECK(avdErrorPaths);
    AndroidDeviceInfoList avdList;
    QStringList avdInfo;
    using ErrorPath = QString;
    using AvdResult = Utils::variant<std::monostate, AndroidDeviceInfo, ErrorPath>;
    const auto parseAvdInfo = [](const QStringList &avdInfo) {
        if (!avdInfo.filter(avdManufacturerError).isEmpty()) {
            for (const QString &line : avdInfo) {
                QString value;
                if (valueForKey(avdInfoPathKey, line, &value))
                    return AvdResult(value); // error path
            }
        } else if (Utils::optional<AndroidDeviceInfo> avd = parseAvd(avdInfo)) {
            // armeabi-v7a devices can also run armeabi code
            if (avd->cpuAbi.contains(Constants::ANDROID_ABI_ARMEABI_V7A))
                avd->cpuAbi << Constants::ANDROID_ABI_ARMEABI;
            avd->state = IDevice::DeviceConnected;
            avd->type = IDevice::Emulator;
            return AvdResult(*avd);
        } else {
            qCDebug(avdOutputParserLog) << "Avd Parsing: Parsing failed: " << avdInfo;
        }
        return AvdResult();
    };

    const auto lines = output.split('\n');
    for (const QString &line : lines) {
        if (line.startsWith("---------") || line.isEmpty()) {
            const AvdResult result = parseAvdInfo(avdInfo);
            if (auto info = Utils::get_if<AndroidDeviceInfo>(&result))
                avdList << *info;
            else if (auto errorPath = Utils::get_if<ErrorPath>(&result))
                *avdErrorPaths << *errorPath;
            avdInfo.clear();
        } else {
            avdInfo << line;
        }
    }

    Utils::sort(avdList);

    return avdList;
}

int platformNameToApiLevel(const QString &platformName)
{
    int apiLevel = -1;
    static const QRegularExpression re("(android-)(?<apiLevel>[0-9A-Z]{1,})",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(platformName);
    if (match.hasMatch()) {
        QString apiLevelStr = match.captured("apiLevel");
        bool isUInt;
        apiLevel = apiLevelStr.toUInt(&isUInt);
        if (!isUInt) {
            if (apiLevelStr == 'Q')
                apiLevel = 29;
            else if (apiLevelStr == 'R')
                apiLevel = 30;
            else if (apiLevelStr == 'S')
                apiLevel = 31;
            else if (apiLevelStr == "Tiramisu")
                apiLevel = 33;
        }
    }
    return apiLevel;
}

} // namespace Internal
} // namespace Android
