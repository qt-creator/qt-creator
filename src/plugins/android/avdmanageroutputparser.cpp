// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "avdmanageroutputparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSettings>

#include <optional>
#include <variant>

namespace {
Q_LOGGING_CATEGORY(avdOutputParserLog, "qtc.android.avdOutputParser", QtWarningMsg)
}

// Avd list keys to parse avd
const char avdInfoNameKey[] = "Name:";
const char avdInfoPathKey[] = "Path:";
const char avdInfoAbiKey[] = "abi.type";
const char avdInfoTargetKey[] = "target";
const char avdInfoErrorKey[] = "Error:";

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

static std::optional<AndroidDeviceInfo> parseAvd(const QStringList &deviceInfo)
{
    AndroidDeviceInfo avd;
    for (const QString &line : deviceInfo) {
        QString value;
        if (valueForKey(avdInfoErrorKey, line)) {
            qCDebug(avdOutputParserLog) << "Avd Parsing: Skip avd device. Error key found:" << line;
            return {};
        } else if (valueForKey(avdInfoNameKey, line, &value)) {
            avd.avdName = value;
        } else if (valueForKey(avdInfoPathKey, line, &value)) {
            const Utils::FilePath avdPath = Utils::FilePath::fromUserInput(value);
            avd.avdPath = avdPath;
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
                const QString avdInfoFileName = avd.avdName + ".ini";
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
        }
    }
    if (avd != AndroidDeviceInfo())
        return avd;
    return {};
}

AndroidDeviceInfoList parseAvdList(const QString &output, Utils::FilePaths *avdErrorPaths)
{
    QTC_CHECK(avdErrorPaths);
    AndroidDeviceInfoList avdList;
    QStringList avdInfo;
    using ErrorPath = Utils::FilePath;
    using AvdResult = std::variant<std::monostate, AndroidDeviceInfo, ErrorPath>;
    const auto parseAvdInfo = [](const QStringList &avdInfo) {
        if (!avdInfo.filter(avdManufacturerError).isEmpty()) {
            for (const QString &line : avdInfo) {
                QString value;
                if (valueForKey(avdInfoPathKey, line, &value))
                    return AvdResult(Utils::FilePath::fromString(value)); // error path
            }
        } else if (std::optional<AndroidDeviceInfo> avd = parseAvd(avdInfo)) {
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
            if (auto info = std::get_if<AndroidDeviceInfo>(&result))
                avdList << *info;
            else if (auto errorPath = std::get_if<ErrorPath>(&result))
                *avdErrorPaths << *errorPath;
            avdInfo.clear();
        } else {
            avdInfo << line;
        }
    }

    return Utils::sorted(std::move(avdList));
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

QString convertNameToExtension(const QString &name)
{
    if (name.endsWith("ext4"))
        return " Extension 4";

    return {};
}

} // namespace Internal
} // namespace Android
