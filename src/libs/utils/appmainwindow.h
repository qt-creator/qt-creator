// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QMainWindow>

namespace Utils {

class QTCREATOR_UTILS_EXPORT AppMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    AppMainWindow();

public slots:
    void raiseWindow();

signals:
    void deviceChange();

#ifdef Q_OS_WIN
protected:
    virtual bool winEvent(MSG *message, long *result);
    bool event(QEvent *event) override;
#endif

private:
    const int m_deviceEventId;
};

} // Utils
