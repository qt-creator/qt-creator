/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
