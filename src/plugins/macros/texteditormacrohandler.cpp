/**************************************************************************
**
** Copyright (C) 2015 Nicolas Arnaud-Cormos
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "texteditormacrohandler.h"
#include "macroevent.h"
#include "macro.h"

#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>

#include <QAction>
#include <QKeyEvent>
#include <QApplication>

using namespace Macros;
using namespace Macros::Internal;

static const char KEYEVENTNAME[] = "TextEditorKey";
static quint8 TEXT = 0;
static quint8 TYPE = 1;
static quint8 MODIFIERS = 2;
static quint8 KEY = 3;
static quint8 AUTOREP = 4;
static quint8 COUNT = 5;


TextEditorMacroHandler::TextEditorMacroHandler():
    IMacroHandler()
{
    const QObject *editorManager = Core::EditorManager::instance();
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(changeEditor(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
            this, SLOT(closeEditor(Core::IEditor*)));
}

void TextEditorMacroHandler::startRecording(Macro *macro)
{
    IMacroHandler::startRecording(macro);
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->installEventFilter(this);

    // Block completion
    Core::ActionManager::command(TextEditor::Constants::COMPLETE_THIS)->action()->blockSignals(true);
}

void TextEditorMacroHandler::endRecordingMacro(Macro *macro)
{
    if (m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->removeEventFilter(this);
    IMacroHandler::endRecordingMacro(macro);

    // Unblock completion
    Core::ActionManager::command(TextEditor::Constants::COMPLETE_THIS)->action()->blockSignals(false);
}

bool TextEditorMacroHandler::canExecuteEvent(const MacroEvent &macroEvent)
{
    return (macroEvent.id() == KEYEVENTNAME);
}

bool TextEditorMacroHandler::executeEvent(const MacroEvent &macroEvent)
{
    if (!m_currentEditor)
        return false;

    QKeyEvent keyEvent((QEvent::Type)macroEvent.value(TYPE).toInt(),
                       macroEvent.value(KEY).toInt(),
                       (Qt::KeyboardModifiers)macroEvent.value(MODIFIERS).toInt(),
                       macroEvent.value(TEXT).toString(),
                       macroEvent.value(AUTOREP).toBool(),
                       macroEvent.value(COUNT).toInt());
    QApplication::instance()->sendEvent(m_currentEditor->widget(), &keyEvent);
    return true;
}

bool TextEditorMacroHandler::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)

    if (!isRecording())
        return false;

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
        MacroEvent e;
        e.setId(KEYEVENTNAME);
        e.setValue(TEXT, keyEvent->text());
        e.setValue(TYPE, keyEvent->type());
        e.setValue(MODIFIERS, (int)keyEvent->modifiers());
        e.setValue(KEY, keyEvent->key());
        e.setValue(AUTOREP, keyEvent->isAutoRepeat());
        e.setValue(COUNT, keyEvent->count());
        addMacroEvent(e);
    }
    return false;
}

void TextEditorMacroHandler::changeEditor(Core::IEditor *editor)
{
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->removeEventFilter(this);

    m_currentEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->installEventFilter(this);
}

void TextEditorMacroHandler::closeEditor(Core::IEditor *editor)
{
    Q_UNUSED(editor);
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->removeEventFilter(this);
    m_currentEditor = 0;
}
