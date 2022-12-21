// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QCursor;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Debugger::Internal {

class DebuggerEngine;
class DebuggerPane;
class CombinedPane;
class InputPane;

class LogWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit LogWindow(DebuggerEngine *engine);
    ~LogWindow() final;

    DebuggerEngine *engine() const;

    void setCursor(const QCursor &cursor);

    QString combinedContents() const;
    QString inputContents() const;

    void clearUndoRedoStacks();

    static QString logTimeStamp();

    void clearContents();
    void sendCommand();
    void executeLine();
    void showOutput(int channel, const QString &output);
    void showInput(int channel, const QString &input);
    void doOutput();
    void repeatLastCommand();

signals:
    void statusMessageRequested(const QString &msg, int);

private:
    CombinedPane *m_combinedText;  // combined input/output
    InputPane *m_inputText;     // scriptable input alone
    QTimer m_outputTimer;
    QString m_queuedOutput;
    Utils::FancyLineEdit *m_commandEdit;
    bool m_ignoreNextInputEcho;
    DebuggerEngine *m_engine;
};

class GlobalLogWindow final : public QWidget
{
public:
    explicit GlobalLogWindow();
    ~GlobalLogWindow() final;

    void setCursor(const QCursor &cursor);

    void clearUndoRedoStacks();
    void clearContents();
    void doInput(const QString &input);
    void doOutput(const QString &output);

private:
    DebuggerPane *m_rightPane; // everything
    DebuggerPane *m_leftPane;  // combined input
};

} // Debugger::Internla
