// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace Utils { class FilePath; }

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

namespace QmlTraceViewer {

// Left-hand sidebar. Currently lists opened traces; meant to grow more sections later.
class MainSidebar : public QWidget
{
    Q_OBJECT

public:
    explicit MainSidebar(QWidget *parent = nullptr);

    // Adds the trace if not present yet and selects it without emitting traceActivated().
    void addTrace(const Utils::FilePath &filePath);

    Utils::FilePath currentTrace() const;

signals:
    void traceActivated(const Utils::FilePath &filePath);

private:
    QListWidget *m_list = nullptr;
};

} // namespace QmlTraceViewer
