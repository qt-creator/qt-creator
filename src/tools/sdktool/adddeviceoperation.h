/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "operation.h"

#include <QString>

extern const char DEVICEMANAGER_ID[];
extern const char DEFAULT_DEVICES_ID[];
extern const char DEVICE_LIST_ID[];

extern const char DEVICE_ID_ID[];

class AddDeviceOperation : public Operation
{
public:
    AddDeviceOperation();

    QString name() const;
    QString helpText() const;
    QString argumentsHelpText() const;

    bool setArguments(const QStringList &args);

    int execute() const;

#ifdef WITH_TESTS
    bool test() const;
#endif

    static QVariantMap addDevice(const QVariantMap &map,
                                 const QString &id, const QString &displayName, int type,
                                 int auth, const QString &hwPlatform, const QString &swPlatform,
                                 const QString &debugServer, const QString &freePorts,
                                 const QString &host, const QString &keyFile,
                                 int origin, const QString &osType, const QString &passwd,
                                 int sshPort, int timeout, const QString &uname, int version,
                                 const KeyValuePairList &extra);

    static QVariantMap initializeDevices();

    static bool exists(const QString &id);
    static bool exists(const QVariantMap &map, const QString &id);

private:
    static KeyValuePairList createDevice(const QString &id, const QString &displayName, int type,
                                         int auth, const QString &hwPlatform, const QString &swPlatform,
                                         const QString &debugServer, const QString &freePorts,
                                         const QString &host, const QString &keyFile,
                                         int origin, const QString &osType, const QString &passwd,
                                         int sshPort, int timeout, const QString &uname, int version,
                                         const KeyValuePairList &extra);

    int m_authentication;
    QString m_b2q_platformHardware;
    QString m_b2q_platformSoftware;
    QString m_debugServer;
    QString m_freePortsSpec;
    QString m_host;
    QString m_id;
    QString m_keyFile;
    QString m_displayName;
    int m_origin;
    QString m_osType;
    QString m_password;
    int m_sshPort;
    int m_timeout;
    int m_type;
    QString m_uname;
    int m_version;
    KeyValuePairList m_extra;
};
