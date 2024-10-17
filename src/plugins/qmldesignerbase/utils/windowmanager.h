// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <QPoint>
#include <QtQml/qqml.h>

QT_FORWARD_DECLARE_CLASS(QRect)
QT_FORWARD_DECLARE_CLASS(QWindow)

namespace QmlDesigner {

class QMLDESIGNERBASE_EXPORT WindowManager : public QObject
{
    Q_OBJECT

public:
    ~WindowManager();

    WindowManager(const WindowManager &) = delete;
    void operator=(const WindowManager &) = delete;

    static void registerDeclarativeType();

    Q_INVOKABLE QPoint globalCursorPosition();
    Q_INVOKABLE QRect getScreenGeometry(QPoint point);

signals:
    void focusWindowChanged(QWindow *window);
    void aboutToQuit();
    void mainWindowVisibleChanged(bool value);

private:
    WindowManager();
};

} // namespace QmlDesigner
