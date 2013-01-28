/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_LOGWINDOW_H
#define DEBUGGER_LOGWINDOW_H

#include "debuggerconstants.h"

#include <QWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QCursor;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Debugger {
namespace Internal {

class DebuggerPane;

class LogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = 0);

    void setCursor(const QCursor &cursor);

    QString combinedContents() const;
    QString inputContents() const;

    static QString logTimeStamp();

    static bool writeLogContents(const QPlainTextEdit *editor, QWidget *parent = 0);

    static QChar charForChannel(int channel);
    static LogChannel channelForChar(QChar c);

public slots:
    void clearContents();
    void sendCommand();
    void executeLine();
    void showOutput(int channel, const QString &output);
    void showInput(int channel, const QString &input);
    void doOutput();

signals:
    void showPage();
    void statusMessageRequested(const QString &msg, int);

private:
    DebuggerPane *m_combinedText;  // combined input/output
    DebuggerPane *m_inputText;     // scriptable input alone
    QTimer m_outputTimer;
    QString m_queuedOutput;
    Utils::FancyLineEdit *m_commandEdit;
    QLabel *m_commandLabel;
    bool m_ignoreNextInputEcho;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LOGWINDOW_H

