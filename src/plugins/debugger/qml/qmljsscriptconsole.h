/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef QMLJSSCRIPTCONSOLE_H
#define QMLJSSCRIPTCONSOLE_H

#include <qmljsdebugclient/qdeclarativeenginedebug.h>
#include <QtGui/QPlainTextEdit>

namespace Utils {
class StatusLabel;
}

namespace Debugger {

class QmlAdapter;

namespace Internal {

class QmlJSScriptConsolePrivate;
class QmlJSScriptConsole;

class QmlJSScriptConsoleWidget : public QWidget
{
    Q_OBJECT
public:
    QmlJSScriptConsoleWidget(QWidget *parent = 0);

    void setQmlAdapter(QmlAdapter *adapter);
    void setInferiorStopped(bool inferiorStopped);

public slots:
    void appendResult(const QString &result);

signals:
    void evaluateExpression(const QString &expr);

private:
    QmlJSScriptConsole *m_console;
    Utils::StatusLabel *m_statusLabel;

};

class QmlJSScriptConsole : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit QmlJSScriptConsole(QWidget *parent = 0);
    ~QmlJSScriptConsole();

    inline void setTitle(const QString &title)
    { setDocumentTitle(title); }

    inline QString title() const
    { return documentTitle(); }

    void setPrompt(const QString &prompt);
    QString prompt() const;

    void setInferiorStopped(bool inferiorStopped);

    void setQmlAdapter(QmlAdapter *adapter);

    void appendResult(const QString &result);

public slots:
    void clear();
    void onStateChanged(QmlJsDebugClient::QDeclarativeDebugQuery::State);
    void onSelectionChanged();
    void insertDebugOutput(QtMsgType type, const QString &debugMsg);

protected:
    void keyPressEvent(QKeyEvent *e);
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseReleaseEvent(QMouseEvent *e);

signals:
    void evaluateExpression(const QString &expr);
    void updateStatusMessage(const QString &message, int timeoutMS);

private slots:
    void onCursorPositionChanged();

private:
    void displayPrompt();
    void handleReturnKey();
    void handleUpKey();
    void handleDownKey();
    void handleHomeKey();
    QString getCurrentScript() const;
    void replaceCurrentScript(const QString &script);
    bool isEditableArea() const;

private:
    QmlJSScriptConsolePrivate *d;
};

} //Internal
} //Debugger

#endif
