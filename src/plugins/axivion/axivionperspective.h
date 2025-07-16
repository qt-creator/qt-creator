// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Axivion::Internal {

void updateDashboard();
void showFilterException(const QString &errorMessage);
void showErrorMessage(const QString &errorMessage);
void reinitDashboard(const QString &projectName);
void resetDashboard();
void updateIssueDetails(const QString &html, const QString &projectName);
void updateNamedFilters();
void updateLocalBuildStateFor(const QString &projectName, const QString &state, int percent);

void leaveOrEnterDashboardMode(bool byLocalBuildButton);
bool currentIssueHasValidPathMapping();
void showLocalBuildProgress();

void setupAxivionPerspective();

} // Axivion::Internal
