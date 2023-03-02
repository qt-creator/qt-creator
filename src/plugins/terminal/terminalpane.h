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

    virtual QWidget *outputWidget(QWidget *parent);
    virtual QList<QWidget *> toolBarWidgets() const;
    virtual QString displayName() const;
    virtual int priorityInStatusBar() const;
    virtual void clearContents();
    virtual void visibilityChanged(bool visible);
    virtual void setFocus();
    virtual bool hasFocus() const;
    virtual bool canFocus() const;
    virtual bool canNavigate() const;
    virtual bool canNext() const;
    virtual bool canPrevious() const;
    virtual void goToNext();
    virtual void goToPrev();

    void openTerminal(const Utils::Terminal::OpenTerminalParameters &parameters);
    void addTerminal(TerminalWidget *terminal, const QString &title);

private:
    TerminalWidget *currentTerminal() const;

    void removeTab(int index);
    void setupTerminalWidget(TerminalWidget *terminal);

private:
    QTabWidget *m_tabWidget{nullptr};

    QToolButton *m_newTerminalButton{nullptr};
    QToolButton *m_closeTerminalButton{nullptr};

    QAction m_newTerminal;
    QAction m_closeTerminal;
    QAction m_nextTerminal;
    QAction m_prevTerminal;
};

} // namespace Terminal
