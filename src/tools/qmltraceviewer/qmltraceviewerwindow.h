// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMainWindow>

namespace Utils { class FilePath; }

namespace QmlTraceViewer {

class WindowPrivate;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);

    void loadTraceFile(const Utils::FilePath &filePath);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    WindowPrivate *d;
};

} // namespace QmlTraceViewer
