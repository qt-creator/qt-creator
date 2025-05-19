// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Axivion::Internal {

struct LocalDashboardAccess
{
    QString user;
    QString password;
    QString url;
};

void checkForLocalBuildResults(const QString &projectName, const std::function<void()> &callback);
void startLocalDashboard(const QString &projectName, const std::function<void()> &callback);
bool shutdownAllLocalDashboards(const std::function<void()> &callback);

std::optional<LocalDashboardAccess> localDashboardAccessFor(const QString &projectName);

bool startLocalBuild(const QString &projectName);
void cancelLocalBuild(const QString &projectName);
bool hasRunningLocalBuild(const QString &projectName);

enum class LocalBuildState { None, Started, Building, Analyzing, UpdatingDashboard, Finished };

struct LocalBuildInfo
{
    LocalBuildState state = LocalBuildState::None;
    QString buildOutput = {};
    QString axivionOutput = {};
};

LocalBuildInfo localBuildInfoFor(const QString &projectName);
void removeFinishedLocalBuilds();

} // namespace Axivion::Internal
