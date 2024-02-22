// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QToolButton;
QT_END_NAMESPACE

namespace Axivion::Internal {

class AxivionOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    explicit AxivionOutputPane(QObject *parent = nullptr);
    ~AxivionOutputPane();

    // IOutputPane interface
    QWidget *outputWidget(QWidget *parent) override;
    QList<QWidget *> toolBarWidgets() const override;
    void clearContents() override;
    void setFocus() override;
    bool hasFocus() const override;
    bool canFocus() const override;
    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;

    void updateDashboard();

private:
    QStackedWidget *m_outputWidget = nullptr;
    QToolButton *m_showDashboard = nullptr;
    QToolButton *m_showIssues = nullptr;
};

} // Axivion::Internal
