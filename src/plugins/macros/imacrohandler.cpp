// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imacrohandler.h"

#include "macro.h"

using namespace Macros::Internal;

/*!
    \class Macro::IEventHandler
    \brief The IEventHandler class is a base class for all macro event handlers.

    An event handler is used to handle a specific type of macro events.
    They are used to create and replay macro events. Use
    MacroManager::registerEventHandler
    to add a new event handler.
*/

/*!
    \fn void IEventHandler::startRecording(Macro* macro)

    Initializes some data when starting to record a macro.
*/

/*!
    \fn void IEventHandler::endRecordingMacro(Macro* macro)

    Cleans up after recording a macro.
*/

/*!
    \fn bool IEventHandler::canExecuteEvent(const MacroEvent &macroEvent)

    When replaying a macro, the manager iterates through all macro events
    specified in \a macroEvent
    in the macro and calls this function to determine which handler to use.
    If the function returns \c true, \c executeEvent is called.
*/

/*!
    \fn bool IEventHandler::executeEvent(const MacroEvent &macroEvent)

    Executes the specified \a macroEvent, using the values stored in
    the macro event.
*/

// ---------- IMacroHandler ------------

void IMacroHandler::startRecording(Macro* macro)
{
    m_currentMacro = macro;
}

void IMacroHandler::endRecordingMacro(Macro* macro)
{
    Q_UNUSED(macro)
    m_currentMacro = nullptr;
}

void IMacroHandler::addMacroEvent(const MacroEvent& event)
{
    if (m_currentMacro)
        m_currentMacro->append(event);
}

void IMacroHandler::setCurrentMacro(Macro *macro)
{
    m_currentMacro = macro;
}

bool IMacroHandler::isRecording() const
{
    return m_currentMacro != nullptr;
}
