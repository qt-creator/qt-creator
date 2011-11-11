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

#include "qmljsscriptconsole.h"
#include "interactiveinterpreter.h"
#include "qmladapter.h"
#include "debuggerstringutils.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/coreconstants.h>
#include <utils/statuslabel.h>

#include <QtGui/QMenu>
#include <QtGui/QTextBlock>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QToolButton>

namespace Debugger {
namespace Internal {

class QmlJSScriptConsolePrivate
{
public:
    QmlJSScriptConsolePrivate()
        : prompt(_("> ")),
          startOfEditableArea(-1),
          lastKnownPosition(0),
          inferiorStopped(false)
    {
        resetCache();
    }

    void resetCache();
    void appendToHistory(const QString &script);
    bool canEvaluateScript(const QString &script);

    QWeakPointer<QmlAdapter> adapter;

    QString prompt;
    int startOfEditableArea;
    int lastKnownPosition;

    QStringList scriptHistory;
    int scriptHistoryIndex;

    InteractiveInterpreter interpreter;

    bool inferiorStopped;
    QList<QTextEdit::ExtraSelection> selections;
};

void QmlJSScriptConsolePrivate::resetCache()
{
    scriptHistory.clear();
    scriptHistory.append(QString());
    scriptHistoryIndex = scriptHistory.count();

    selections.clear();
}

void QmlJSScriptConsolePrivate::appendToHistory(const QString &script)
{
    scriptHistoryIndex = scriptHistory.count();
    scriptHistory.replace(scriptHistoryIndex - 1,script);
    scriptHistory.append(QString());
    scriptHistoryIndex = scriptHistory.count();
}

bool QmlJSScriptConsolePrivate::canEvaluateScript(const QString &script)
{
    interpreter.clearText();
    interpreter.appendText(script);
    return interpreter.canEvaluate();
}

///////////////////////////////////////////////////////////////////////
//
// QmlJSScriptConsoleWidget
//
///////////////////////////////////////////////////////////////////////

QmlJSScriptConsoleWidget::QmlJSScriptConsoleWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    QWidget *statusbarContainer = new QWidget;

    QHBoxLayout *hbox = new QHBoxLayout(statusbarContainer);
    hbox->setMargin(0);
    hbox->setSpacing(0);

    //Clear Button
    QToolButton *clearButton = new QToolButton;
    QAction *clearAction = new QAction(tr("Clear Console"), this);
    clearAction->setIcon(QIcon(_(Core::Constants::ICON_CLEAN_PANE)));

    clearButton->setDefaultAction(clearAction);

    //Status Label
    m_statusLabel = new Utils::StatusLabel;

    hbox->addWidget(m_statusLabel, 20, Qt::AlignLeft);
    hbox->addWidget(clearButton, 0, Qt::AlignRight);

    m_console = new QmlJSScriptConsole;
    connect(m_console, SIGNAL(evaluateExpression(QString)), this,
            SIGNAL(evaluateExpression(QString)));
    connect(m_console, SIGNAL(updateStatusMessage(const QString &, int)), m_statusLabel,
            SLOT(showStatusMessage(const QString &, int)));
    connect(clearAction, SIGNAL(triggered()), m_console, SLOT(clear()));
    vbox->addWidget(statusbarContainer);
    vbox->addWidget(m_console);

}

void QmlJSScriptConsoleWidget::setQmlAdapter(QmlAdapter *adapter)
{
    m_console->setQmlAdapter(adapter);
}

void QmlJSScriptConsoleWidget::setInferiorStopped(bool inferiorStopped)
{
    m_console->setInferiorStopped(inferiorStopped);
}

void QmlJSScriptConsoleWidget::appendResult(const QString &result)
{
    m_console->appendResult(result);
}

///////////////////////////////////////////////////////////////////////
//
// QmlJSScriptConsole
//
///////////////////////////////////////////////////////////////////////

QmlJSScriptConsole::QmlJSScriptConsole(QWidget *parent)
    : QPlainTextEdit(parent),
      d(new QmlJSScriptConsolePrivate())
{
    connect(this, SIGNAL(cursorPositionChanged()), SLOT(onCursorPositionChanged()));

    setFrameStyle(QFrame::NoFrame);
    setUndoRedoEnabled(false);
    setBackgroundVisible(false);
    const TextEditor::FontSettings &fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    setFont(fs.font());

    displayPrompt();
}

QmlJSScriptConsole::~QmlJSScriptConsole()
{
    delete d;
}

void QmlJSScriptConsole::setPrompt(const QString &prompt)
{
    d->prompt = prompt;
}

QString QmlJSScriptConsole::prompt() const
{
    return d->prompt;
}

void QmlJSScriptConsole::setInferiorStopped(bool inferiorStopped)
{
    d->inferiorStopped = inferiorStopped;
    onSelectionChanged();
}

void QmlJSScriptConsole::setQmlAdapter(QmlAdapter *adapter)
{
    d->adapter = adapter;
    clear();
}

void QmlJSScriptConsole::appendResult(const QString &result)
{
    QString currentScript = getCurrentScript();
    d->appendToHistory(currentScript);

    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertText(_("\n"));
    cur.insertText(result);
    cur.movePosition(QTextCursor::EndOfLine);
    cur.insertText(_("\n"));
    setTextCursor(cur);
    displayPrompt();

    QTextEdit::ExtraSelection sel;

    QTextCharFormat resultFormat;
    resultFormat.setForeground(QBrush(QColor(Qt::darkGray)));

    QTextCursor c(document()->findBlockByNumber(cur.blockNumber()-1));
    c.movePosition(QTextCursor::StartOfBlock);
    c.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);

    sel.format = resultFormat;
    sel.cursor = c;

    d->selections.append(sel);

    setExtraSelections(d->selections);
}

void QmlJSScriptConsole::clear()
{
    d->resetCache();

    QPlainTextEdit::clear();
    displayPrompt();
}

void QmlJSScriptConsole::onStateChanged(QmlJsDebugClient::QDeclarativeDebugQuery::State state)
{
    QDeclarativeDebugExpressionQuery *query = qobject_cast<QDeclarativeDebugExpressionQuery *>(sender());

    if (query && state != QDeclarativeDebugQuery::Error) {
        QString result(query->result().toString());
        if (result == _("<undefined>") && d->inferiorStopped) {
            //don't give up. check if we can still evaluate using javascript engine
            emit evaluateExpression(getCurrentScript());
        } else {
            appendResult(result);
        }
    } else {
        QPlainTextEdit::appendPlainText(QString());
        moveCursor(QTextCursor::EndOfLine);
    }
    delete query;
}

void QmlJSScriptConsole::onSelectionChanged()
{
    if (!d->adapter.isNull()) {
        QString status;
        if (!d->inferiorStopped) {
            status.append(tr("Current Selected Object: "));
            status.append(d->adapter.data()->currentSelectedDisplayName());
        }
        emit updateStatusMessage(status, 0);
    }
}

void QmlJSScriptConsole::keyPressEvent(QKeyEvent *e)
{
    bool keyConsumed = false;
    switch (e->key()) {

    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (isEditableArea()) {
            handleReturnKey();
            keyConsumed = true;
        }
        break;

    case Qt::Key_Backspace: {
        QTextCursor cursor = textCursor();
        bool hasSelection = cursor.hasSelection();
        int selectionStart = cursor.selectionStart();
        if ((hasSelection && selectionStart < d->startOfEditableArea)
                || (!hasSelection && selectionStart == d->startOfEditableArea)) {
            keyConsumed = true;
        }
        break;
    }

    case Qt::Key_Delete:
        if (textCursor().selectionStart() < d->startOfEditableArea) {
            keyConsumed = true;
        }
        break;

    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        keyConsumed = true;
        break;

    case Qt::Key_Left:
        if (textCursor().position() == d->startOfEditableArea) {
            keyConsumed = true;
        } else if (e->modifiers() & Qt::ControlModifier && isEditableArea()) {
            handleHomeKey();
            keyConsumed = true;
        }
        break;

    case Qt::Key_Up:
        if (isEditableArea()) {
            handleUpKey();
            keyConsumed = true;
        }
        break;

    case Qt::Key_Down:
        if (isEditableArea()) {
            handleDownKey();
            keyConsumed = true;
        }
        break;

    case Qt::Key_Home:
        if (isEditableArea()) {
            handleHomeKey();
            keyConsumed = true;
        }
        break;

    case Qt::Key_C:
    case Qt::Key_Insert: {
        //Fair to assume that for any selection beyond startOfEditableArea
        //only copy function is allowed.
        QTextCursor cursor = textCursor();
        bool hasSelection = cursor.hasSelection();
        int selectionStart = cursor.selectionStart();
        if (hasSelection && selectionStart < d->startOfEditableArea) {
            if (!(e->modifiers() & Qt::ControlModifier))
                keyConsumed = true;
        }
        break;
    }

    default: {
        QTextCursor cursor = textCursor();
        bool hasSelection = cursor.hasSelection();
        int selectionStart = cursor.selectionStart();
        if (hasSelection && selectionStart < d->startOfEditableArea) {
            keyConsumed = true;
        }
        break;
    }
    }

    if (!keyConsumed)
        QPlainTextEdit::keyPressEvent(e);
}

void QmlJSScriptConsole::contextMenuEvent(QContextMenuEvent *event)
{
    QTextCursor cursor = textCursor();
    Qt::TextInteractionFlags flags = textInteractionFlags();
    bool hasSelection = cursor.hasSelection();
    int selectionStart = cursor.selectionStart();
    bool canBeEdited = true;
    if (hasSelection && selectionStart < d->startOfEditableArea) {
        canBeEdited = false;
    }

    QMenu *menu = new QMenu();
    QAction *a;

    if ((flags & Qt::TextEditable) && canBeEdited) {
        a = menu->addAction(tr("Cut"), this, SLOT(cut()));
        a->setEnabled(cursor.hasSelection());
    }

    a = menu->addAction(tr("Copy"), this, SLOT(copy()));
    a->setEnabled(cursor.hasSelection());

    if ((flags & Qt::TextEditable) && canBeEdited) {
        a = menu->addAction(tr("Paste"), this, SLOT(paste()));
        a->setEnabled(canPaste());
    }

    menu->addSeparator();
    a = menu->addAction(tr("Select All"), this, SLOT(selectAll()));
    a->setEnabled(!document()->isEmpty());

    menu->addSeparator();
    menu->addAction(tr("Clear"), this, SLOT(clear()));

    menu->exec(event->globalPos());

    delete menu;
}

void QmlJSScriptConsole::mouseReleaseEvent(QMouseEvent *e)
{
    QPlainTextEdit::mouseReleaseEvent(e);
    QTextCursor cursor = textCursor();
    if (e->button() == Qt::LeftButton && !cursor.hasSelection() && !isEditableArea()) {
        cursor.setPosition(d->lastKnownPosition);
        setTextCursor(cursor);
    }
}

void QmlJSScriptConsole::onCursorPositionChanged()
{
    if (!isEditableArea()) {
        setTextInteractionFlags(Qt::TextSelectableByMouse);
    } else {
        d->lastKnownPosition = textCursor().position();
        setTextInteractionFlags(Qt::TextEditorInteraction);
    }
}

void QmlJSScriptConsole::displayPrompt()
{
    d->startOfEditableArea = textCursor().position() + d->prompt.length();
    QTextCursor cur = textCursor();
    cur.insertText(d->prompt);
    cur.movePosition(QTextCursor::EndOfWord);
    setTextCursor(cur);
}

void QmlJSScriptConsole::handleReturnKey()
{
    QString currentScript = getCurrentScript();
    bool scriptEvaluated = false;

    //Check if string is only white spaces
    if (currentScript.trimmed().isEmpty()) {
        QTextCursor cur = textCursor();
        cur.movePosition(QTextCursor::EndOfLine);
        cur.insertText(_("\n"));
        setTextCursor(cur);
        displayPrompt();
        scriptEvaluated = true;
    }

    if (!scriptEvaluated) {
        //check if it can be evaluated
        if (d->canEvaluateScript(currentScript)) {

            //Select the engine for evaluation based on
            //inferior state
            if (!d->inferiorStopped) {
                if (!d->adapter.isNull()) {
                    QDeclarativeEngineDebug *engineDebug = d->adapter.data()->engineDebugClient();
                    int id = d->adapter.data()->currentSelectedDebugId();
                    if (engineDebug && id != -1) {
                        QDeclarativeDebugExpressionQuery *query =
                                engineDebug->queryExpressionResult(id, currentScript, this);
                        connect(query, SIGNAL(stateChanged(QmlJsDebugClient::QDeclarativeDebugQuery::State)),
                                this, SLOT(onStateChanged(QmlJsDebugClient::QDeclarativeDebugQuery::State)));
                        scriptEvaluated = true;
                    }
                }
            }

            if (!scriptEvaluated) {
                emit evaluateExpression(currentScript);
                scriptEvaluated = true;
            }
        }
    }
    if (!scriptEvaluated) {
        QPlainTextEdit::appendPlainText(QString());
        moveCursor(QTextCursor::EndOfLine);
    }

}

void QmlJSScriptConsole::handleUpKey()
{
    //get the current script and update in script history
    QString currentScript = getCurrentScript();
    d->scriptHistory.replace(d->scriptHistoryIndex - 1,currentScript);

    if (d->scriptHistoryIndex > 1)
        d->scriptHistoryIndex--;

    replaceCurrentScript(d->scriptHistory.at(d->scriptHistoryIndex - 1));
}

void QmlJSScriptConsole::handleDownKey()
{
    //get the current script and update in script history
    QString currentScript = getCurrentScript();
    d->scriptHistory.replace(d->scriptHistoryIndex - 1,currentScript);

    if (d->scriptHistoryIndex < d->scriptHistory.count())
        d->scriptHistoryIndex++;

    replaceCurrentScript(d->scriptHistory.at(d->scriptHistoryIndex - 1));
}

void QmlJSScriptConsole::handleHomeKey()
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(d->startOfEditableArea);
    setTextCursor(cursor);
}

QString QmlJSScriptConsole::getCurrentScript() const
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(d->startOfEditableArea);
    while (cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor)) ;
    QString script = cursor.selectedText();
    cursor.clearSelection();
    //remove trailing white space
    int end = script.size() - 1;
    while (end > 0 && script[end].isSpace())
        end--;
    return script.left(end + 1);
}

void QmlJSScriptConsole::replaceCurrentScript(const QString &script)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(d->startOfEditableArea);
    while (cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor)) ;
    cursor.deleteChar();
    cursor.insertText(script);
    setTextCursor(cursor);
}

bool QmlJSScriptConsole::isEditableArea() const
{
    return textCursor().position() >= d->startOfEditableArea;
}

} //Internal
} //Debugger
