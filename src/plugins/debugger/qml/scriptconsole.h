/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
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
