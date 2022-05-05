/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <utils/filepath.h>
#include <utils/id.h>

QT_BEGIN_NAMESPACE
class QSettings;
class QJsonObject;
QT_END_NAMESPACE

namespace GitLab {

class GitLabServer
{
public:
    enum { defaultPort = 443 };

    GitLabServer(); // TODO different protocol handling e.g. for clone / push?
    GitLabServer(const Utils::Id &id, const QString &host, const QString &description,
                 const QString &token, unsigned short port);;
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

    bool validateCert = true; // TODO
};

class GitLabParameters
{
public:
    GitLabParameters();

    bool equals(const GitLabParameters &other) const;
    bool isValid() const;

    void toSettings(QSettings *s) const;
    void fromSettings(const QSettings *s);

    GitLabServer currentDefaultServer() const;
    GitLabServer serverForId(const Utils::Id &id) const;

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
