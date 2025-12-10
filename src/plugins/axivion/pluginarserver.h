// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

QT_BEGIN_NAMESPACE
template <typename T> class QList;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace Axivion::Internal {

using CallbackFunc = std::function<void()>;
using SessionCallbackFunc = std::function<void(int)>;

void startPluginArServer(const Utils::FilePath &bauhausSuite, const CallbackFunc &onRunning);
void cleanShutdownPluginArServer(const Utils::FilePath &bauhausSuite);
void shutdownPluginArServer(const Utils::FilePath &bauhausSuite);
void shutdownAllPluginArServers();

void requestArSessionStart(const Utils::FilePath &bauhausSuite,
                           const SessionCallbackFunc &onStarted);
void requestArSessionFinish(const Utils::FilePath &bauhausSuite, int sessionId, bool abort);
void requestIssuesDisposal(const Utils::FilePath &bauhausSuite, int sessionId,
                           const QList<long> &issues);

QString pluginArPipeOut(const Utils::FilePath &bauhausSuite, int sessionId);

} // namespace Axivion::Internal

