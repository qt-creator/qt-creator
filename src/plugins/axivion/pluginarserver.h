// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

namespace Utils { class FilePath; }

namespace Axivion::Internal {

using OnServerStarted = std::function<void()>;
using OnServerStartFailed = std::function<void()>;
using OnSessionStarted = std::function<void(int)>;

void startPluginArServer(const Utils::FilePath &bauhausSuite, const OnServerStarted &onRunning,
                         const OnServerStartFailed &onFailed);
void cleanShutdownPluginArServer(const Utils::FilePath &bauhausSuite);
void shutdownPluginArServer(const Utils::FilePath &bauhausSuite);
void shutdownAllPluginArServers();

void requestArSessionStart(const Utils::FilePath &bauhausSuite, const OnSessionStarted &onStarted);
void requestArSessionFinish(const Utils::FilePath &bauhausSuite, int sessionId, bool abort);
void requestIssuesDisposal(const Utils::FilePath &bauhausSuite, const QList<qint64> &issues);

void fetchIssueInfoFromPluginAr(const Utils::FilePath &bauhausSuite, const QString &relativeIssue);

QString pluginArPipeOut(const Utils::FilePath &bauhausSuite, int sessionId);

} // namespace Axivion::Internal

