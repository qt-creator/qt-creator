/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "debuggerconstants.h"

#include <QWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QCursor;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Debugger {
namespace Internal {

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
    Q_OBJECT

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

} // namespace Internal
} // namespace Debugger
