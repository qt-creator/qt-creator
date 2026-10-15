// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <utils/outputformat.h>

#include <optional>

namespace Utils { class FilePath; }

namespace Axivion::Internal {

enum class DashboardMode;

void updateDashboard();
void showFilterException(const QString &errorMessage);
void showErrorMessage(const QString &errorMessage);
void reinitDashboard(const QString &projectName);
void resetDashboard();
void updateIssueDetails(const QString &html, const QString &projectName,
                        std::optional<DashboardMode> dashboardMode);
void updateNamedFilters();
void updateLocalBuildStateFor(const QString &projectName, const QString &state, int percent);
void updateSfaStateFor(const Utils::FilePath &fileName, const QString &state, int percent);
void appendLocalBuildOutputFor(const QString &projectName, const QString &output,
                               Utils::OutputFormat format);
void appendSfaOutputFor(const Utils::FilePath &fileName, const QString &output,
                        Utils::OutputFormat format);
void resetLocalBuildConsole(const QString &projectName);
void resetSfaConsole(const Utils::FilePath &fileName);

void leaveOrEnterDashboardMode(bool byLocalBuildButton);
bool currentIssueHasValidPathMapping();
bool localChangesSelected();
void showLocalBuildProgress();
void showIssueDetails();

void setupAxivionPerspective();

} // Axivion::Internal
