// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>
#include <utils/id.h>

#include <QtGlobal>

#include <solutions/tasking/tasktreerunner.h>

QT_BEGIN_NAMESPACE
class QJsonObject;
QT_END_NAMESPACE

namespace Axivion::Internal {

constexpr char s_axivionKeychainService[] = "keychain.axivion.qtcreator";

class AxivionServer
{
public:
    QString displayString() const { return username + " @ " + dashboard; }
    bool operator==(const AxivionServer &other) const;
    bool operator!=(const AxivionServer &other) const;

    QJsonObject toJson() const;
    static AxivionServer fromJson(const QJsonObject &json);

    // id starts empty == invalid; set to generated uuid on first apply of valid dashboard config,
    // never changes afterwards
    Utils::Id id;
    QString dashboard;
    QString username;

    bool validateCert = true;
};

class PathMapping
{
public:
    bool operator==(const PathMapping &other) const;
    bool operator!=(const PathMapping &other) const;

    bool isValid() const;
    QString projectName;
    Utils::FilePath analysisPath;
    Utils::FilePath localPath;
};

class AxivionSettings : public Utils::AspectContainer
{
public:
    AxivionSettings();

    void toSettings() const;

    Utils::Id defaultDashboardId() const;
    const AxivionServer defaultServer() const;
    const AxivionServer serverForId(const Utils::Id &id) const;
    void disableCertificateValidation(const Utils::Id &id);
    const QList<AxivionServer> allAvailableServers() const { return m_allServers; };
    bool updateDashboardServers(const QList<AxivionServer> &other, const Utils::Id &selected);
    const QList<PathMapping> validPathMappings() const;

    Utils::BoolAspect highlightMarks{this};
private:
    Utils::StringAspect m_defaultServerId{this};
    QList<AxivionServer> m_allServers;
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

AxivionSettings &settings();

QString credentialKey(const AxivionServer &server);

} // Axivion::Internal
