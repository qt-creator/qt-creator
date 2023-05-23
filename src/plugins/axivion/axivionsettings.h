// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include <utils/filepath.h>
#include <utils/id.h>

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QJsonObject;
class QSettings;
QT_END_NAMESPACE

namespace Axivion::Internal {

class AxivionServer
{
public:
    AxivionServer() = default;
    AxivionServer(const Utils::Id &id, const QString &dashboardUrl,
                  const QString &description, const QString &token);

    bool operator==(const AxivionServer &other) const;
    bool operator!=(const AxivionServer &other) const;

    QJsonObject toJson() const;
    static AxivionServer fromJson(const QJsonObject &json);
    QStringList curlArguments() const;

    Utils::Id id;
    QString dashboard;
    QString description;
    QString token;

    bool validateCert = true;
};

class AxivionSettings
{
public:
    AxivionSettings();
    void toSettings(QSettings *s) const;
    void fromSettings(QSettings *s);

    AxivionServer server; // shall we have more than one?
    Utils::FilePath curl;
};

} // Axivion::Internal
