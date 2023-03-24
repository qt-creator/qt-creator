// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/ioutputpane.h>

#include <utils/terminalhooks.h>

#include <QAction>
#include <QTabWidget>
#include <QToolButton>

namespace Terminal {

class TerminalWidget;

class TerminalPane : public Core::IOutputPane
{
    Q_OBJECT
public:
    TerminalPane(QObject *parent = nullptr);
    ~TerminalPane() override;

    QWidget *outputWidget(QWidget *parent) override;
    QList<QWidget *> toolBarWidgets() const override;
    QString displayName() const override;
    int priorityInStatusBar() const override;
    void clearContents() override;
    void setFocus() override;
    bool hasFocus() const override;
    bool canFocus() const override;
    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;

    void openTerminal(const Utils::Terminal::OpenTerminalParameters &parameters);
    void addTerminal(TerminalWidget *terminal, const QString &title);

    TerminalWidget *stoppedTerminalWithId(const Utils::Id &identifier) const;

private:
    TerminalWidget *currentTerminal() const;

    void removeTab(int index);
    void setupTerminalWidget(TerminalWidget *terminal);

private:
    QTabWidget *m_tabWidget{nullptr};

    QToolButton *m_newTerminalButton{nullptr};
    QToolButton *m_closeTerminalButton{nullptr};
    QToolButton *m_openSettingsButton{nullptr};
    QToolButton *m_escSettingButton{nullptr};

    bool m_widgetInitialized{false};
};

} // namespace Terminal
