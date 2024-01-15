// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicectlutils.h"

#include "iostr.h"

#include <QJsonDocument>

Utils::expected_str<QJsonValue> Ios::Internal::parseDevicectlResult(const QByteArray &rawOutput)
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
        return Utils::make_unexpected(
            Tr::tr("Failed to parse devicectl output: %1.").arg(parseError.errorString()));
    }
    const QJsonValue errorValue = jsonOutput["error"];
    if (!errorValue.isUndefined()) {
        // error
        QString error
            = Tr::tr("Operation failed: %1.")
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
        return Utils::make_unexpected(error);
    }
    const QJsonValue resultValue = jsonOutput["result"];
    if (resultValue.isUndefined()) {
        return Utils::make_unexpected(
            Tr::tr("Failed to parse devicectl output: 'result' is missing"));
    }
    return resultValue;
}
