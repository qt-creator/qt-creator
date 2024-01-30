// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

QString parseRuleInfo(const QByteArray &input);

} // ResultParser

} // Axivion::Internal
