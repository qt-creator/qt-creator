// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <utils/filepath.h>

namespace Utils { class Process; }

namespace ProjectExplorer {

enum SshHostKeyCheckingMode {
    SshHostKeyCheckingNone,
    SshHostKeyCheckingStrict,
    SshHostKeyCheckingAllowNoMatch,
};

class PROJECTEXPLORER_EXPORT SshParameters
{
public:
    enum AuthenticationType {
        AuthenticationTypeAll,
        AuthenticationTypeSpecificKey,
    };

    SshParameters();

    QString host() const { return m_host; }
    quint16 port() const { return m_port; }
    QString userName() const { return m_userName; }

    QString userAtHost() const;
    QString userAtHostAndPort() const;

    void setHost(const QString &host) { m_host = host; }
    void setPort(int port) { m_port = port; }
    void setUserName(const QString &name) { m_userName = name; }

    QStringList connectionOptions(const Utils::FilePath &binary) const;

    Utils::FilePath privateKeyFile;
    QString x11DisplayName;
    int timeout = 0; // In seconds.
    AuthenticationType authenticationType = AuthenticationTypeAll;
    SshHostKeyCheckingMode hostKeyCheckingMode = SshHostKeyCheckingAllowNoMatch;

    static bool setupSshEnvironment(Utils::Process *process);

    friend PROJECTEXPLORER_EXPORT bool operator==(const SshParameters &p1, const SshParameters &p2);
    friend bool operator!=(const SshParameters &p1, const SshParameters &p2) { return !(p1 == p2); }

private:
    QString m_host;
    quint16 m_port = 22;
    QString m_userName;
};

#ifdef WITH_TESTS
namespace SshTest {
const QString PROJECTEXPLORER_EXPORT getHostFromEnvironment();
quint16 PROJECTEXPLORER_EXPORT getPortFromEnvironment();
const QString PROJECTEXPLORER_EXPORT getUserFromEnvironment();
const QString PROJECTEXPLORER_EXPORT getKeyFileFromEnvironment();
const PROJECTEXPLORER_EXPORT QString userAtHost();
const PROJECTEXPLORER_EXPORT QString userAtHostAndPort();
SshParameters PROJECTEXPLORER_EXPORT getParameters();
bool PROJECTEXPLORER_EXPORT checkParameters(const SshParameters &params);
void PROJECTEXPLORER_EXPORT printSetupHelp();
} // namespace SshTest
#endif

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::SshParameters::AuthenticationType)
