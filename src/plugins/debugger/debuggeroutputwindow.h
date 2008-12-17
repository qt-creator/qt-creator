/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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

