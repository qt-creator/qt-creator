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

#ifndef QMLJSSCRIPTCONSOLE_H
#define QMLJSSCRIPTCONSOLE_H

#include <QtGui/QWidget>
#include <QtGui/QToolButton>
#include <QtGui/QPlainTextEdit>

#include <utils/fancylineedit.h>

namespace QmlJSEditor {
class Highlighter;
}

namespace Debugger {
namespace Internal {

class ScriptConsole : public QWidget
{
    Q_OBJECT
public:
    ScriptConsole(QWidget *parent = 0);

public slots:
    void appendResult(const QString &result);
signals:
    void expressionEntered(const QString &expr);

protected slots:
    void clearTextEditor();
    void executeExpression();

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    void setFontSettings();
    void clear();

//    QToolButton *m_clearButton;
    QPlainTextEdit *m_textEdit;
    Utils::FancyLineEdit *m_lineEdit;
    QString m_prompt;
    QString m_expr;
    QString m_lastExpr;

    QString m_title;
    QmlJSEditor::Highlighter *m_highlighter;

};


}
} //end namespaces


#endif
