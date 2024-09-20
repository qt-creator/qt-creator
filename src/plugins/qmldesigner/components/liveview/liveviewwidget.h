// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSharedMemory>
#include <QWidget>
#include <QTimer>

namespace QmlDesigner {

class LiveViewWidget : public QWidget
{
    Q_OBJECT
public:
    LiveViewWidget(QWidget *parent = nullptr);

private:
    void resizePreview();

    QSharedMemory m_sharedMemory; // Hack to communicate with live preview puppet
    QTimer m_timer;
    QRect m_lastRect;
};

} //namespace QmlDesigner
