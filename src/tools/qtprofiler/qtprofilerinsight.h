// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace QtProfiler {

// Whether this build can send usage statistics at all: Qt Insight Tracker was
// found, and a server and token were configured. Everything below does nothing
// otherwise.
bool usageStatisticsAvailable();

// Starts the tracker if the user has agreed to usage statistics, asking once if
// they have not been asked yet.
void initUsageStatistics(QWidget *dialogParent);
void setUsageStatisticsEnabled(bool enabled);
void shutdownUsageStatistics();

} // namespace QtProfiler
