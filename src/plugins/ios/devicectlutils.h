// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/result.h>

#include <QJsonValue>

namespace Utils {
struct FilePathInfo;
class FilePath;
} // namespace Utils

namespace Ios::Internal {

const char kDeviceName[] = "deviceName";
const char kDeveloperStatus[] = "developerStatus";
const char kDeviceConnected[] = "deviceConnected";
const char kOsVersion[] = "osVersion";
const char kProductType[] = "productType";
const char kCpuArchitecture[] = "cpuArchitecture";
const char kUniqueDeviceId[] = "uniqueDeviceId";
const char vOff[] = "*off*";
const char vDevelopment[] = "Development";
const char vYes[] = "YES";

Utils::Result<QJsonValue> parseDevicectlResult(const QByteArray &rawOutput);
Utils::Result<> checkDevicectlResult(const QByteArray &rawOutput);
Utils::Result<QMap<QString, QString>> parseDeviceInfo(const QByteArray &rawOutput,
                                                            const QString &deviceUsbId);
Utils::Result<QUrl> parseAppInfo(const QByteArray &rawOutput, const QString &bundleIdentifier);
Utils::Result<qint64> parseProcessIdentifier(const QByteArray &rawOutput);
Utils::Result<qint64> parseLaunchResult(const QByteArray &rawOutput);
Utils::Result<QSet<QString>> parseAppIdentifiers(const QByteArray &rawOutput);
Utils::Result<QMap<Utils::FilePath, Utils::FilePathInfo>> parseFileList(
    const QByteArray &rawOutput,
    const Utils::FilePath &basePath,
    bool includeSubDirectories = false);

} // namespace Ios::Internal
