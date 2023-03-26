// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <utils/filepath.h>

#include <QUrl>

namespace Utils { class QtcProcess; }

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

    QString host() const { return url.host(); }
    quint16 port() const { return url.port(); }
    QString userName() const { return url.userName(); }
    QString userAtHost() const { return userName().isEmpty() ? host() : userName() + '@' + host(); }
    void setHost(const QString &host) { url.setHost(host); }
    void setPort(int port) { url.setPort(port); }
    void setUserName(const QString &name) { url.setUserName(name); }

    QStringList connectionOptions(const Utils::FilePath &binary) const;

    QUrl url;
    Utils::FilePath privateKeyFile;
    QString x11DisplayName;
    int timeout = 0; // In seconds.
    AuthenticationType authenticationType = AuthenticationTypeAll;
    SshHostKeyCheckingMode hostKeyCheckingMode = SshHostKeyCheckingAllowNoMatch;

    static bool setupSshEnvironment(Utils::QtcProcess *process);
};

PROJECTEXPLORER_EXPORT bool operator==(const SshParameters &p1, const SshParameters &p2);
PROJECTEXPLORER_EXPORT bool operator!=(const SshParameters &p1, const SshParameters &p2);

#ifdef WITH_TESTS
namespace SshTest {
const QString PROJECTEXPLORER_EXPORT getHostFromEnvironment();
quint16 PROJECTEXPLORER_EXPORT getPortFromEnvironment();
const QString PROJECTEXPLORER_EXPORT getUserFromEnvironment();
const QString PROJECTEXPLORER_EXPORT getKeyFileFromEnvironment();
const PROJECTEXPLORER_EXPORT QString userAtHost();
SshParameters PROJECTEXPLORER_EXPORT getParameters();
bool PROJECTEXPLORER_EXPORT checkParameters(const SshParameters &params);
void PROJECTEXPLORER_EXPORT printSetupHelp();
} // namespace SshTest
#endif

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::SshParameters::AuthenticationType)
