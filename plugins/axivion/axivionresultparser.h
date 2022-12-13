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

namespace ResultParser {

DashboardInfo parseDashboardInfo(const QByteArray &input);

} // ResultParser

} // Axivion::Internal
