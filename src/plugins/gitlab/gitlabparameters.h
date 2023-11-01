// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/id.h>

QT_BEGIN_NAMESPACE
class QJsonObject;
QT_END_NAMESPACE

namespace Utils { class QtcSettings; }

namespace GitLab {

class GitLabServer
{
public:
    enum { defaultPort = 443 };

    GitLabServer(); // TODO different protocol handling e.g. for clone / push?
    GitLabServer(const Utils::Id &id, const QString &host, const QString &description,
                 const QString &token, unsigned short port, bool secure);
    bool operator==(const GitLabServer &other) const;
    bool operator!=(const GitLabServer &other) const;
    QJsonObject toJson() const;
    static GitLabServer fromJson(const QJsonObject &json);
    QStringList curlArguments() const;
    QString displayString() const;

    Utils::Id id;
    QString host;
    QString description;
    QString token;
    unsigned short port = 0;

    bool secure = true;
    bool validateCert = true;
};

class GitLabParameters : public QObject
{
    Q_OBJECT

public:
    GitLabParameters();

    void assign(const GitLabParameters &other);
    bool equals(const GitLabParameters &other) const;
    bool isValid() const;

    void toSettings(Utils::QtcSettings *s) const;
    void fromSettings(const Utils::QtcSettings *s);

    GitLabServer currentDefaultServer() const;
    GitLabServer serverForId(const Utils::Id &id) const;

signals:
    void changed();

public:
    friend bool operator==(const GitLabParameters &p1, const GitLabParameters &p2)
    {
        return p1.equals(p2);
    }
    friend bool operator!=(const GitLabParameters &p1, const GitLabParameters &p2)
    {
        return !p1.equals(p2);
    }

    Utils::Id defaultGitLabServer;
    QList<GitLabServer> gitLabServers;
    Utils::FilePath curl;
};

} // namespace GitLab

Q_DECLARE_METATYPE(GitLab::GitLabServer)
