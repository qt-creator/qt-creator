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
#ifndef EXPRESSIONQUERYWIDGET_H
#define EXPRESSIONQUERYWIDGET_H

#include <private/qdeclarativedebug_p.h>

#include <QtGui/qwidget.h>

QT_BEGIN_NAMESPACE

class QGroupBox;
class QPlainTextEdit;
class QLineEdit;
class QToolButton;

QT_END_NAMESPACE

namespace Utils {
    class FancyLineEdit;
}

namespace Core {
    class IContext;
}

namespace QmlJSEditor {
    class Highlighter;
}

namespace Qml {
namespace Internal {

class ExpressionQueryWidget : public QWidget
{
    Q_OBJECT
public:
    enum Mode {
        SeparateEntryMode,
        ShellMode
    };

    ExpressionQueryWidget(Mode mode = SeparateEntryMode, QDeclarativeEngineDebug *client = 0, QWidget *parent = 0);

    void createCommands(Core::IContext *context);
    void setEngineDebug(QDeclarativeEngineDebug *client);
    void clear();

signals:
    void contextHelpIdChanged(const QString &contextHelpId);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

public slots:
    void setCurrentObject(const QDeclarativeDebugObjectReference &obj);

private slots:
    void clearTextEditor();
    void executeExpression();
    void showResult();
    void invokeCompletion();
    void changeContextHelpId(const QString &text);

private:
    void setFontSettings();
    void appendPrompt();
    void checkCurrentContext();
    void showCurrentContext();
    void updateTitle();

    Mode m_mode;

    QDeclarativeEngineDebug *m_client;
    QDeclarativeDebugExpressionQuery *m_query;
    QPlainTextEdit *m_textEdit;
    Utils::FancyLineEdit *m_lineEdit;

    QToolButton *m_clearButton;
    QString m_prompt;
    QString m_expr;
    QString m_lastExpr;

    QString m_title;
    QmlJSEditor::Highlighter *m_highlighter;

    QDeclarativeDebugObjectReference m_currObject;
    QDeclarativeDebugObjectReference m_objectAtLastFocus;
};

} // Internal
} // Qml

#endif

