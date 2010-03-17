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
#include "expressionquerywidget.h"
#include "qmlinspectorconstants.h"

#include <utils/fancylineedit.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <qmljseditor/qmljshighlighter.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <QtCore/QEvent>
#include <QtGui/QTextCharFormat>
#include <QtGui/QKeySequence>
#include <QtGui/QLabel>
#include <QtGui/QTextEdit>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QGroupBox>
#include <QtGui/QTextObject>
#include <QtGui/QLayout>
#include <QtGui/QShortcut>

#include <QtCore/QDebug>

ExpressionQueryWidget::ExpressionQueryWidget(Mode mode, QDeclarativeEngineDebug *client, QWidget *parent)
    : QWidget(parent),
      m_mode(mode),
      m_client(client),
      m_query(0),
      m_textEdit(new QPlainTextEdit),
      m_lineEdit(0)
{
    m_prompt = QLatin1String(">");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_textEdit);
    m_textEdit->setFrameStyle(QFrame::NoFrame);

    updateTitle();

    m_highlighter = new QmlJSEditor::Highlighter(m_textEdit->document());
    m_highlighter->setParent(m_textEdit->document());

    if (m_mode == SeparateEntryMode) {
        m_lineEdit = new QLineEdit;

        m_lineEdit->setPlaceholderText(tr("<Expression>"));
        connect(m_lineEdit, SIGNAL(returnPressed()), SLOT(executeExpression()));
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->setMargin(1);
        hbox->setSpacing(1);
        //hbox->addWidget(new QLabel(tr("Expression:")));
        hbox->addWidget(m_lineEdit);
        layout->addLayout(hbox);

        m_textEdit->setReadOnly(true);
        m_lineEdit->installEventFilter(this);
    } else {
        m_textEdit->installEventFilter(this);
        appendPrompt();
    }
    setFontSettings();
}

void ExpressionQueryWidget::setFontSettings()
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
    m_highlighter->setFormats(formats.constBegin(), formats.constEnd());
    m_highlighter->rehighlight();
    m_textEdit->setFont(fs.font());
    if (m_mode == SeparateEntryMode)
        m_lineEdit->setFont(fs.font());
}

void ExpressionQueryWidget::createCommands(Core::IContext *context)
{
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    // Add shortcut for invoking automatic completion
    QShortcut *completionShortcut = new QShortcut(m_lineEdit);
    completionShortcut->setWhatsThis(tr("Triggers a completion in this scope"));
    // Make sure the shortcut still works when the completion widget is active
    completionShortcut->setContext(Qt::ApplicationShortcut);
    Core::Command *command = am->registerShortcut(completionShortcut, Qml::Constants::COMPLETE_THIS, context->context());
#ifndef Q_WS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Space")));
#else
    command->setDefaultKeySequence(QKeySequence(tr("Meta+Space")));
#endif
    connect(completionShortcut, SIGNAL(activated()), this, SLOT(invokeCompletion()));
}

void ExpressionQueryWidget::invokeCompletion()
{
    qDebug() << "TODO autocomplete";
}

void ExpressionQueryWidget::setEngineDebug(QDeclarativeEngineDebug *client)
{
    m_client = client;
}

void ExpressionQueryWidget::clear()
{
    m_textEdit->clear();
    if (m_lineEdit)
        m_lineEdit->clear();
    if (m_mode == ShellMode)
        appendPrompt();
}

void ExpressionQueryWidget::updateTitle()
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
}

void ExpressionQueryWidget::appendPrompt()
{
    m_textEdit->moveCursor(QTextCursor::End);

    if (m_mode == SeparateEntryMode) {
        m_textEdit->insertPlainText("\n");
    } else {
        m_textEdit->appendPlainText(m_prompt);
    }
}

void ExpressionQueryWidget::setCurrentObject(const QDeclarativeDebugObjectReference &obj)
{
    m_currObject = obj;
    updateTitle();
}

void ExpressionQueryWidget::checkCurrentContext()
{
    m_textEdit->moveCursor(QTextCursor::End);

    if (m_currObject.debugId() != -1 && m_currObject.debugId() != m_objectAtLastFocus.debugId())
        showCurrentContext();
    m_objectAtLastFocus = m_currObject;
}

void ExpressionQueryWidget::showCurrentContext()
{
    if (m_mode == ShellMode) {
        // clear the initial prompt
        if (m_textEdit->document()->lineCount() == 1)
            m_textEdit->clear();
    }

    m_textEdit->moveCursor(QTextCursor::End);
    m_textEdit->appendPlainText(m_currObject.className()
            + QLatin1String(": ")
            + (m_currObject.name().isEmpty() ? QLatin1String("<unnamed object>") : m_currObject.name()));
    appendPrompt();
}

void ExpressionQueryWidget::executeExpression()
{
    if (!m_client)
        return;

    if (m_mode == SeparateEntryMode)
        m_expr = m_lineEdit->text().trimmed();
    else
        m_expr = m_expr.trimmed();

    if (!m_expr.isEmpty() && m_currObject.debugId() != -1) {
        if (m_query)
            delete m_query;
        m_query = m_client->queryExpressionResult(m_currObject.debugId(), m_expr, this);
        if (!m_query->isWaiting())
            showResult();
        else
            QObject::connect(m_query, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                            this, SLOT(showResult()));

        m_lastExpr = m_expr;
        if (m_lineEdit)
            m_lineEdit->clear();
    }
}

void ExpressionQueryWidget::showResult()
{
    if (m_query) {
        m_textEdit->moveCursor(QTextCursor::End);
        QVariant value = m_query->result();
        QString result;

        if (value.type() == QVariant::List || value.type() == QVariant::StringList) {
            result = tr("<%1 items>", "%1 = number of items").arg(value.toList().count());
        } else if (value.isNull()) {
            result = QLatin1String("<no value>");
        } else {
            result = value.toString();
        }

        if (m_mode == SeparateEntryMode) {
            m_textEdit->insertPlainText(m_expr + " : ");
            m_textEdit->insertPlainText(result);
        } else {
            m_textEdit->insertPlainText(" => ");
            m_textEdit->insertPlainText(result);
        }
        appendPrompt();
        m_expr.clear();
    }
}

bool ExpressionQueryWidget::eventFilter(QObject *obj, QEvent *event)
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
                    if (cursor.columnNumber() <= m_prompt.count())
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
                checkCurrentContext();
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
                checkCurrentContext();
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(obj, event);
}
