// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

#include <QString>

extern const char DEVICEMANAGER_ID[];
extern const char DEFAULT_DEVICES_ID[];
extern const char DEVICE_LIST_ID[];

extern const char DEVICE_ID_ID[];

class AddDeviceData
{
public:
    QVariantMap addDevice(const QVariantMap &map) const;

    static QVariantMap initializeDevices();

    static bool exists(const QString &id);
    static bool exists(const QVariantMap &map, const QString &id);

    QString m_id;
    QString m_displayName;
    int m_type = -1;
    int m_authentication = -1;
    QString m_b2q_platformHardware;
    QString m_b2q_platformSoftware;
    QString m_debugServer;
    QString m_freePortsSpec;
    QString m_host;
    QString m_keyFile;
    int m_origin = 1;
    QString m_osType;
    QString m_password;
    int m_sshPort = 0;
    int m_timeout = 5;
    QString m_uname;
    int m_version = 0;
    QStringList m_dockerMappedPaths;
    QString m_dockerRepo;
    QString m_dockerTag;
    QString m_clangdExecutable;
    KeyValuePairList m_extra;
};

class AddDeviceOperation : public Operation, public AddDeviceData
{
public:
    QString name() const final;
    QString helpText() const final;
    QString argumentsHelpText() const final;

    bool setArguments(const QStringList &args) final;

    int execute() const final;

#ifdef WITH_TESTS
    static void unittest();
#endif
};
