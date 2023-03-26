// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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


TextEditorMacroHandler::TextEditorMacroHandler()
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &TextEditorMacroHandler::changeEditor);
    connect(editorManager, &Core::EditorManager::editorAboutToClose,
            this, &TextEditorMacroHandler::closeEditor);
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
    QCoreApplication::sendEvent(m_currentEditor->widget(), &keyEvent);
    return true;
}

bool TextEditorMacroHandler::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)

    if (!isRecording())
        return false;

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        auto keyEvent = dynamic_cast<QKeyEvent *>(event);
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
    Q_UNUSED(editor)
    if (isRecording() && m_currentEditor && m_currentEditor->widget())
        m_currentEditor->widget()->removeEventFilter(this);
    m_currentEditor = nullptr;
}
