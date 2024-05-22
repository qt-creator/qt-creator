// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>
#include <utils/id.h>

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QJsonObject;
QT_END_NAMESPACE

namespace Axivion::Internal {

class AxivionServer
{
public:
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

class AxivionSettings : public Utils::AspectContainer
{
public:
    AxivionSettings();

    void toSettings() const;

    Utils::Id defaultDashboardId() const;
    const AxivionServer defaultServer() const;
    const AxivionServer serverForId(const Utils::Id &id) const;
    void disableCertificateValidation(const Utils::Id &id);
    void modifyDashboardServer(const Utils::Id &id, const AxivionServer &other);

    Utils::BoolAspect highlightMarks{this};
private:
    AxivionServer m_server; // shall we have more than one?
};

AxivionSettings &settings();

} // Axivion::Internal
