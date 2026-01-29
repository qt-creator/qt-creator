// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fancymainwindow.h>

#include <QFuture>

namespace Utils {
class FilePath;
}

namespace QmlTraceViewer {

class WindowPrivate;

class Window : public Utils::FancyMainWindow
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);

    QFuture<void> loadTraceFile(const Utils::FilePath &file);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    WindowPrivate *d;
};

} // namespace QmlTraceViewer
