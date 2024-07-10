// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicectlutils.h"

#include "iostr.h"

#include <QJsonArray>
#include <QJsonDocument>

using namespace Utils;

namespace Ios::Internal {

expected_str<QJsonValue> parseDevicectlResult(const QByteArray &rawOutput)
{
    // there can be crap (progress info) at front and/or end
    const int firstCurly = rawOutput.indexOf('{');
    const int start = std::max(firstCurly, 0);
    const int lastCurly = rawOutput.lastIndexOf('}');
    const int end = lastCurly >= 0 ? lastCurly : rawOutput.size() - 1;
    QJsonParseError parseError;
    auto jsonOutput = QJsonDocument::fromJson(rawOutput.sliced(start, end - start + 1), &parseError);
    if (jsonOutput.isNull()) {
        // parse error
        return make_unexpected(
            Tr::tr("Failed to parse devicectl output: %1.").arg(parseError.errorString()));
    }
    const QJsonValue errorValue = jsonOutput["error"];
    if (!errorValue.isUndefined()) {
        // error
        QString error
            //: The error message (%1) can contain a full stop, so do not add it here
            = Tr::tr("Operation failed: %1")
                  .arg(errorValue["userInfo"]["NSLocalizedDescription"]["string"].toString());
        const QJsonValue userInfo
            = errorValue["userInfo"]["NSUnderlyingError"]["error"]["userInfo"];
        const QList<QJsonValue> moreInfo{userInfo["NSLocalizedDescription"]["string"],
                                         userInfo["NSLocalizedFailureReason"]["string"],
                                         userInfo["NSLocalizedRecoverySuggestion"]["string"]};
        for (const QJsonValue &v : moreInfo) {
            if (!v.isUndefined())
                error += "\n" + v.toString();
        }
        return make_unexpected(error);
    }
    const QJsonValue resultValue = jsonOutput["result"];
    if (resultValue.isUndefined()) {
        return make_unexpected(Tr::tr("Failed to parse devicectl output: \"result\" is missing."));
    }
    return resultValue;
}

expected_str<QMap<QString, QString>> parseDeviceInfo(const QByteArray &rawOutput,
                                                     const QString &deviceUsbId)
{
    const expected_str<QJsonValue> result = parseDevicectlResult(rawOutput);
    if (!result)
        return make_unexpected(result.error());
    // find device
    const QJsonArray deviceList = (*result)["devices"].toArray();
    for (const QJsonValue &device : deviceList) {
        const QString udid = device["hardwareProperties"]["udid"].toString();
        // USB identifiers don't have dashes, but iOS device udids can. Remove.
        if (QString(udid).remove('-') == deviceUsbId) {
            // fill in the map that we use for the iostool data
            QMap<QString, QString> info;
            info[kDeviceName] = device["deviceProperties"]["name"].toString();
            info[kDeveloperStatus] = QLatin1String(
                device["deviceProperties"]["developerModeStatus"] == "enabled" ? vDevelopment
                                                                               : vOff);
            info[kDeviceConnected] = vYes; // that's the assumption
            info[kOsVersion] = QLatin1String("%1 (%2)")
                                   .arg(device["deviceProperties"]["osVersionNumber"].toString(),
                                        device["deviceProperties"]["osBuildUpdate"].toString());
            info[kProductType] = device["hardwareProperties"]["productType"].toString();
            info[kCpuArchitecture] = device["hardwareProperties"]["cpuType"]["name"].toString();
            info[kUniqueDeviceId] = udid;
            return info;
        }
    }
    // device not found, not handled by devicectl
    // not translated, only internal logging
    return make_unexpected(QLatin1String("Device is not handled by devicectl"));
}

Utils::expected_str<QUrl> parseAppInfo(const QByteArray &rawOutput, const QString &bundleIdentifier)
{
    const Utils::expected_str<QJsonValue> result = parseDevicectlResult(rawOutput);
    if (!result)
        return make_unexpected(result.error());
    const QJsonArray apps = (*result)["apps"].toArray();
    for (const QJsonValue &app : apps) {
        if (app["bundleIdentifier"].toString() == bundleIdentifier)
            return QUrl(app["url"].toString());
    }
    return {};
}

Utils::expected_str<qint64> parseProcessIdentifier(const QByteArray &rawOutput)
{
    const expected_str<QJsonValue> result = parseDevicectlResult(rawOutput);
    if (!result)
        return make_unexpected(result.error());
    const QJsonArray matchingProcesses = (*result)["runningProcesses"].toArray();
    if (matchingProcesses.size() > 0)
        return matchingProcesses.first()["processIdentifier"].toInteger(-1);
    return -1;
}

Utils::expected_str<qint64> parseLaunchResult(const QByteArray &rawOutput)
{
    const Utils::expected_str<QJsonValue> result = parseDevicectlResult(rawOutput);
    if (!result)
        return make_unexpected(result.error());
    const qint64 pid = (*result)["process"]["processIdentifier"].toInteger(-1);
    if (pid < 0) {
        // something unexpected happened ...
        return make_unexpected(Tr::tr("devicectl returned unexpected output ... running failed."));
    }
    return pid;
}

} // namespace Ios::Internal
