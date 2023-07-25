// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dashboard/dto.h"

#include <utils/expected.h>

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

class ShortIssue : public BaseResult
{
public:
    QString id;
    QString state;
    QString errorNumber;
    QString message;
    QString entity;
    QString filePath;
    QString severity;
    int lineNumber = 0;
};

class IssuesList : public BaseResult
{
public:
    QList<ShortIssue> issues;
};

namespace ResultParser {

DashboardInfo parseDashboardInfo(const QByteArray &input);
Utils::expected_str<Dto::ProjectInfoDto> parseProjectInfo(const QByteArray &input);
IssuesList parseIssuesList(const QByteArray &input);
QString parseRuleInfo(const QByteArray &input);

} // ResultParser

} // Axivion::Internal
