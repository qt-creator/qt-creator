/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_OUTPUTWINDOW_H
#define DEBUGGER_OUTPUTWINDOW_H

#include <QtGui/QLineEdit>
#include <QtGui/QSplitter>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

namespace Debugger {
namespace Internal {

class DebuggerOutputWindow : public QWidget
{
    Q_OBJECT

public:
    DebuggerOutputWindow(QWidget *parent = 0);

    QWidget *outputWidget(QWidget *) { return this; }
    QList<QWidget*> toolBarWidgets(void) const { return QList<QWidget *>(); }

    QString name() const { return windowTitle(); }
    void visibilityChanged(bool /*visible*/) {}

    void bringPaneToForeground() { emit showPage(); }
    void setCursor(const QCursor &cursor);
    
    QString combinedContents() const;
    QString inputContents() const;

public slots:
    void clearContents();
    void showOutput(const QString &prefix, const QString &output);
    void showInput(const QString &prefix, const QString &input);

signals:
    void showPage();
    void statusMessageRequested(const QString &msg, int);
    void commandExecutionRequested(const QString &cmd);

private slots:
    void onReturnPressed();

private:
    QTextEdit *m_combinedText;  // combined input/output
    QTextEdit *m_inputText;     // scriptable input alone
    QLineEdit *m_commandEdit;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_OUTPUTWINDOW_H

