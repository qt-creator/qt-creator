// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/expected.h>

#include <QJsonValue>

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

Utils::expected_str<QJsonValue> parseDevicectlResult(const QByteArray &rawOutput);
Utils::expected_str<QMap<QString, QString>> parseDeviceInfo(const QByteArray &rawOutput,
                                                            const QString &deviceUsbId);
Utils::expected_str<QUrl> parseAppInfo(const QByteArray &rawOutput, const QString &bundleIdentifier);
Utils::expected_str<qint64> parseProcessIdentifier(const QByteArray &rawOutput);
Utils::expected_str<qint64> parseLaunchResult(const QByteArray &rawOutput);

} // namespace Ios::Internal
