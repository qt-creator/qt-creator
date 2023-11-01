// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "shellintegration.h"
#include "shortcutmap.h"

#include <aggregation/aggregate.h>

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icontext.h>
#include <coreplugin/terminal/searchableterminal.h>

#include <utils/link.h>
#include <utils/process.h>
#include <utils/terminalhooks.h>

#include <QFutureWatcher>

#include <memory>

namespace Terminal {

using RegisteredAction = std::unique_ptr<QAction, std::function<void(QAction *)>>;

class TerminalWidget : public Core::SearchableTerminal
{
    Q_OBJECT
public:
    TerminalWidget(QWidget *parent = nullptr,
                   const Utils::Terminal::OpenTerminalParameters &openParameters = {});

    void closeTerminal();

    void setShellName(const QString &shellName);
    QString shellName() const;
    QString title() const;

    Utils::FilePath cwd() const;
    Utils::CommandLine currentCommand() const;
    std::optional<Utils::Id> identifier() const;
    QProcess::ProcessState processState() const;

    void restart(const Utils::Terminal::OpenTerminalParameters &openParameters);

    static void initActions();

    void unlockGlobalAction(const Utils::Id &commandId);

signals:
    void started(qint64 pid);
    void cwdChanged(const Utils::FilePath &cwd);
    void commandChanged(const Utils::CommandLine &cmd);
    void titleChanged();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    bool event(QEvent *event) override;

    void onReadyRead(bool forceFlush);
    void setupFont();
    void setupPty();
    void setupColors();
    void setupActions();

    void handleEscKey(QKeyEvent *event);

    void surfaceChanged() override;

    void selectionChanged(const std::optional<Selection> &newSelection) override;
    void linkActivated(const Link &link) override;
    void contextMenuRequested(const QPoint &pos) override;

    qint64 writeToPty(const QByteArray &data) override;
    void resizePty(QSize newSize) override;
    void setClipboard(const QString &text) override;
    std::optional<TerminalView::Link> toLink(const QString &text) override;

    RegisteredAction registerAction(Utils::Id commandId, const Core::Context &context);
    void registerShortcut(Core::Command *command);

    void updateCopyState();

private:
    Core::Context m_context;
    std::unique_ptr<Utils::Process> m_process;
    std::unique_ptr<ShellIntegration> m_shellIntegration;

    QString m_shellName;
    QString m_title;

    Utils::Id m_identifier;

    Utils::Terminal::OpenTerminalParameters m_openParameters;

    Utils::FilePath m_cwd;
    Utils::CommandLine m_currentCommand;

    RegisteredAction m_copy;
    RegisteredAction m_paste;
    RegisteredAction m_clearSelection;
    RegisteredAction m_clearTerminal;
    RegisteredAction m_moveCursorWordLeft;
    RegisteredAction m_moveCursorWordRight;
    RegisteredAction m_close;

    Internal::ShortcutMap m_shortcutMap;

    std::unique_ptr<QFutureWatcher<Utils::expected_str<Utils::FilePath>>> m_findShellWatcher;
};

} // namespace Terminal
