/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_CONSOLEWINDOW_H
#define DEBUGGER_CONSOLEWINDOW_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QCursor;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class Console;

class ConsoleWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWindow(QWidget *parent = 0);

    void setCursor(const QCursor &cursor);

    QString combinedContents() const;
    QString inputContents() const;

    static QString logTimeStamp();

public slots:
    void clearContents();
    void showOutput(int channel, const QString &output);
    void showInput(int channel, const QString &input);

signals:
    void showPage();
    void statusMessageRequested(const QString &msg, int);

private:
    Console *m_console;  // combined input/output
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CONSOLEWINDOW_H

