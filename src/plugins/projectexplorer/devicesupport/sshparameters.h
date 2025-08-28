// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <utils/aspects.h>
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

    static void setupSshEnvironment(Utils::Process *process);

    friend PROJECTEXPLORER_EXPORT bool operator==(const SshParameters &p1, const SshParameters &p2);
    friend bool operator!=(const SshParameters &p1, const SshParameters &p2) { return !(p1 == p2); }

    Utils::FilePath privateKeyFile() const { return m_privateKeyFile; }
    void setPrivateKeyFile(const Utils::FilePath &file) { m_privateKeyFile = file; }

    QString x11DisplayName() const { return m_x11DisplayName; }
    void setX11DisplayName(const QString &display) { m_x11DisplayName = display; }

    int timeout() const { return m_timeout; }
    void setTimeout(int seconds) { m_timeout = seconds; }

    AuthenticationType authenticationType() const { return m_authenticationType; }
    void setAuthenticationType(AuthenticationType type) { m_authenticationType = type; }

    SshHostKeyCheckingMode hostKeyCheckingMode() const { return m_hostKeyCheckingMode; }
    void setHostKeyCheckingMode(SshHostKeyCheckingMode mode) { m_hostKeyCheckingMode = mode; }

private:
    Utils::FilePath m_privateKeyFile;
    QString m_x11DisplayName;
    int m_timeout = 0; // In seconds. TODO: Change to chrono::duration
    AuthenticationType m_authenticationType = AuthenticationTypeAll;
    SshHostKeyCheckingMode m_hostKeyCheckingMode = SshHostKeyCheckingAllowNoMatch;

    QString m_host;
    quint16 m_port = 22;
    QString m_userName;
};

class PROJECTEXPLORER_EXPORT SshParametersAspectContainer : public Utils::AspectContainer
{
public:
    SshParametersAspectContainer();

    SshParameters sshParameters() const;
    void setSshParameters(const SshParameters &params);

public:
    Utils::BoolAspect useKeyFile{this};
    Utils::FilePathAspect privateKeyFile{this};
    Utils::IntegerAspect timeout{this};
    Utils::TypedSelectionAspect<SshHostKeyCheckingMode> hostKeyCheckingMode{this};

    Utils::StringAspect host{this};
    Utils::IntegerAspect port{this};
    Utils::StringAspect userName{this};
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
