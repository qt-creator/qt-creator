// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

class CrashHandler;
class CrashHandlerDialogPrivate;

class CrashHandlerDialog : public QDialog
{
    Q_OBJECT

public:
    CrashHandlerDialog(CrashHandler *handler, const QString &signalName,
                       const QString &appName, QWidget *parent = nullptr);
    ~CrashHandlerDialog();

public:
    void setApplicationInfo(const QString &signalName, const QString &appName);
    void appendDebugInfo(const QString &chunk);
    void selectLineWithContents(const QString &text);
    void setToFinalState();
    void disableRestartAppCheckBox();
    void disableDebugAppButton();
    bool runDebuggerWhileBacktraceNotFinished();

private:
    CrashHandlerDialogPrivate *d;
};
