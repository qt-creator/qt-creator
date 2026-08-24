// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtprofilertypes.h"

#include <QWidget>

#include <chrono>

namespace Utils { class FilePath; }

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

namespace QtProfiler {

// Left-hand sidebar. Currently lists opened traces; meant to grow more sections later.
class MainSidebar : public QWidget
{
    Q_OBJECT

public:
    explicit MainSidebar(QWidget *parent = nullptr);

    // Adds the trace if not present yet and selects it without emitting traceActivated().
    void addTrace(const Utils::FilePath &filePath);

    void setTraceFormat(const Utils::FilePath &filePath, Format format);
    void setTraceDuration(const Utils::FilePath &filePath, std::chrono::milliseconds ms);

    // Removes the currently selected trace. Removing it selects a neighbour (which
    // emits traceActivated()); returns true if a trace remains selected afterwards.
    bool removeCurrentTrace();

    Utils::FilePath currentTrace() const;

signals:
    void traceActivated(const Utils::FilePath &filePath);

private:
    QListWidgetItem *traceItem(const Utils::FilePath &filePath) const;

    QListWidget *m_list = nullptr;
};

} // namespace QtProfiler
