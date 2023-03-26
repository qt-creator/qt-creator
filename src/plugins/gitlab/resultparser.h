// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMetaType>
#include <QString>

namespace GitLab {

struct Error
{
    int code = 200; // below 200: internal, 200+ HTTP
    QString message;
};

class PageInformation
{
public:
    int currentPage = -1;
    int totalPages = -1;
    int perPage = -1;
    int total = -1;
};

class User
{
public:
    QString name;
    QString realname;
    QString email;
    QString lastLogin;
    Error error;
    int id = -1;
    bool bot = false;
};

class Project
{
public:
    QString name;
    QString displayName;
    QString pathName;
    QString visibility;
    QString httpUrl;
    QString sshUrl;
    Error error;
    int id = -1;
    int starCount = -1;
    int forkCount = -1;
    int issuesCount = -1;
    int accessLevel = -1; // 50 owner, 40 maintainer, 30 developer, 20 reporter, 10 guest
};

class Projects
{
public:
    QList<Project> projects;
    Error error;
    PageInformation pageInfo;
};

class Event
{
public:
    QString action;
    QString targetType;
    QString targetTitle;
    QString timeStamp;
    QString pushData;
    User author;
    Error error;

    QString toMessage() const;
};

class Events
{
public:
    QList<Event> events;
    Error error;
    PageInformation pageInfo;
};

namespace ResultParser {

User parseUser(const QByteArray &input);
Project parseProject(const QByteArray &input);
Projects parseProjects(const QByteArray &input);
Events parseEvents(const QByteArray &input);
Error parseErrorMessage(const QString &message);

} // namespace ResultParser
} // namespace GitLab

Q_DECLARE_METATYPE(GitLab::Project)
