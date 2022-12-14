// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include <QList>

namespace Axivion::Internal {

class BaseResult
{
public:
    QString error;
};

class Project : public BaseResult
{
public:
    QString name;
    QString url;
};

class DashboardInfo : public BaseResult
{
public:
    QString mainUrl;
    QList<Project> projects;
};

class User : public BaseResult
{
public:
    QString name;
    QString displayName;
    enum UserType { Dashboard, Virtual, Unknown } type;
};

class IssueKind : public BaseResult
{
public:
    QString prefix;
    QString niceSingular;
    QString nicePlural;
};

class IssueCount : public BaseResult
{
public:
    QString issueKind;
    int total = 0;
    int added = 0;
    int removed = 0;
};

class ResultVersion : public BaseResult
{
public:
    QString name;
    QString timeStamp;
    QList<IssueCount> issueCounts;
    int linesOfCode = 0;
};

class ProjectInfo : public BaseResult
{
public:
    QString name;
    QList<User> users;
    QList<ResultVersion> versions;
    QList<IssueKind> issueKinds;
};

namespace ResultParser {

DashboardInfo parseDashboardInfo(const QByteArray &input);
ProjectInfo parseProjectInfo(const QByteArray &input);

} // ResultParser

} // Axivion::Internal
