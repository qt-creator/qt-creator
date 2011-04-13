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

#include "scriptconsole.h"

#include <QtCore/QDebug>
#include <QtGui/QVBoxLayout>
#include <QtGui/QDockWidget>
#include <qmljseditor/qmljshighlighter.h>
#include <utils/styledbar.h>
#include <utils/filterlineedit.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

namespace Debugger {
namespace Internal {

ScriptConsole::ScriptConsole(QWidget *parent)
    : QWidget(parent),
      m_textEdit(new QPlainTextEdit),
      m_lineEdit(0)
{

//    m_prompt = QLatin1String(">");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_textEdit);
    m_textEdit->setFrameStyle(QFrame::NoFrame);

    //updateTitle();

    /*m_highlighter = new QmlJSEditor::Highlighter(m_textEdit->document());
    m_highlighter->setParent(m_textEdit->document());*/

    Utils::StyledBar *bar = new Utils::StyledBar;
    m_lineEdit = new Utils::FilterLineEdit;

    m_lineEdit->setPlaceholderText(tr("<Type expression to evaluate>"));
    m_lineEdit->setToolTip(tr("Write and evaluate QtScript expressions."));

    /*m_clearButton = new QToolButton();
    m_clearButton->setToolTip(tr("Clear Output"));
    m_clearButton->setIcon(QIcon(Core::Constants::ICON_CLEAN_PANE));
    connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clearTextEditor()));*/
    //connect(m_lineEdit, SIGNAL(textChanged(QString)), SLOT(changeContextHelpId(QString)));

    connect(m_lineEdit, SIGNAL(returnPressed()), SLOT(executeExpression()));
    QHBoxLayout *hbox = new QHBoxLayout(bar);
    hbox->setMargin(1);
    hbox->setSpacing(1);
    hbox->addWidget(m_lineEdit);
    //hbox->addWidget(m_clearButton);
    layout->addWidget(bar);

    m_textEdit->setReadOnly(true);
    m_lineEdit->installEventFilter(this);

    setFontSettings();
}


void ScriptConsole::setFontSettings()
{
    const TextEditor::FontSettings &fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    static QVector<QString> categories;
    if (categories.isEmpty()) {
        categories << QLatin1String(TextEditor::Constants::C_NUMBER)
                << QLatin1String(TextEditor::Constants::C_STRING)
                << QLatin1String(TextEditor::Constants::C_TYPE)
                << QLatin1String(TextEditor::Constants::C_KEYWORD)
                << QLatin1String(TextEditor::Constants::C_LABEL)
                << QLatin1String(TextEditor::Constants::C_COMMENT)
                << QLatin1String(TextEditor::Constants::C_VISUAL_WHITESPACE);
    }

    const QVector<QTextCharFormat> formats = fs.toTextCharFormats(categories);
/*    m_highlighter->setFormats(formats);
    m_highlighter->rehighlight();*/
    m_textEdit->setFont(fs.font());
    m_lineEdit->setFont(fs.font());
}


void ScriptConsole::clear()
{
    clearTextEditor();

    if (m_lineEdit)
        m_lineEdit->clear();
//    appendPrompt();
}

void ScriptConsole::clearTextEditor()
{
    m_textEdit->clear();
    m_textEdit->appendPlainText(tr("Script Console\n"));
}


/*void ExpressionQueryWidget::updateTitle()
{
    if (m_currObject.debugId() < 0) {
        m_title = tr("Expression queries");
    } else {
        QString desc = QLatin1String("<")
            + m_currObject.className() + QLatin1String(": ")
            + (m_currObject.name().isEmpty() ? QLatin1String("<unnamed>") : m_currObject.name())
            + QLatin1String(">");
        m_title = tr("Expression queries (using context for %1)" , "Selected object").arg(desc);
    }
}*/
/*
void ExpressionQueryWidget::appendPrompt()
{
    m_textEdit->moveCursor(QTextCursor::End);

    if (m_mode == SeparateEntryMode) {
        m_textEdit->insertPlainText("\n");
    } else {
        m_textEdit->appendPlainText(m_prompt);
    }
}
*/



bool ScriptConsole::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_textEdit) {
        switch (event->type()) {
            case QEvent::KeyPress:
            {
                QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
                int key = keyEvent->key();
                if (key == Qt::Key_Return || key == Qt::Key_Enter) {
                    executeExpression();
                    return true;
                } else if (key == Qt::Key_Backspace) {
                    // ensure m_expr doesn't contain backspace characters
                    QTextCursor cursor = m_textEdit->textCursor();
                    bool atLastLine = !(cursor.block().next().isValid());
                    if (!atLastLine)
                        return true;
                    if (cursor.positionInBlock() <= m_prompt.count())
                        return true;
                    cursor.deletePreviousChar();
                    m_expr = cursor.block().text().mid(m_prompt.count());
                    return true;
                } else {
                    m_textEdit->moveCursor(QTextCursor::End);
                    m_expr += keyEvent->text();
                }
                break;
            }
            case QEvent::FocusIn:
                //checkCurrentContext();
                m_textEdit->moveCursor(QTextCursor::End);
                break;
            default:
                break;
        }
    } else if (obj == m_lineEdit) {
        switch (event->type()) {
            case QEvent::KeyPress:
            {
                QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
                int key = keyEvent->key();
                if (key == Qt::Key_Up && m_lineEdit->text() != m_lastExpr) {
                    m_expr = m_lineEdit->text();
                    if (!m_lastExpr.isEmpty())
                        m_lineEdit->setText(m_lastExpr);
                } else if (key == Qt::Key_Down) {
                    m_lineEdit->setText(m_expr);
                }
                break;
            }
            case QEvent::FocusIn:
               // checkCurrentContext();
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ScriptConsole::executeExpression()
{
    m_expr = m_lineEdit->text().trimmed();
    m_expr = m_expr.trimmed();
    if (!m_expr.isEmpty()) {
        emit expressionEntered(m_expr);
        m_lastExpr = m_expr;
        if (m_lineEdit)
            m_lineEdit->clear();
    }
}

void ScriptConsole::appendResult(const QString& result)
{
    m_textEdit->moveCursor(QTextCursor::End);
    m_textEdit->insertPlainText(m_expr + " : ");
    m_textEdit->insertPlainText(result);
    m_textEdit->insertPlainText("\n");
    m_expr.clear();
}

}
}
