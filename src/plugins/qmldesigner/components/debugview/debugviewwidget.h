// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

#include "ui_debugviewwidget.h"

namespace QmlDesigner {

namespace Internal {

class DebugViewWidget : public QWidget
{
    Q_OBJECT
public:
    DebugViewWidget(QWidget *parent = nullptr);

    void addLogMessage(const QString &topic, const QString &message, bool highlight = false);
    void addErrorMessage(const QString &topic, const QString &message);
    void addLogInstanceMessage(const QString &topic, const QString &message, bool highlight = false);
    void setPuppetStatus(const QString &text);

    void setDebugViewEnabled(bool b);

    void enabledCheckBoxToggled(bool b);

private:
    Ui::DebugViewWidget m_ui;
};

} //namespace Internal

} //namespace QmlDesigner
