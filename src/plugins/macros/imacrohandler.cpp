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

class IMacroHandler::IMacroHandlerPrivate
{
public:
    IMacroHandlerPrivate();

    Macro *currentMacro;
};

IMacroHandler::IMacroHandlerPrivate::IMacroHandlerPrivate() :
    currentMacro(0)
{
}


// ---------- IMacroHandler ------------

IMacroHandler::IMacroHandler():
    d(new IMacroHandlerPrivate)
{
}

IMacroHandler::~IMacroHandler()
{
    delete d;
}

void IMacroHandler::startRecording(Macro* macro)
{
    d->currentMacro = macro;
}

void IMacroHandler::endRecordingMacro(Macro* macro)
{
    Q_UNUSED(macro)
    d->currentMacro = 0;
}

void IMacroHandler::addMacroEvent(const MacroEvent& event)
{
    if (d->currentMacro)
        d->currentMacro->append(event);
}

void IMacroHandler::setCurrentMacro(Macro *macro)
{
    d->currentMacro = macro;
}

bool IMacroHandler::isRecording() const
{
    return d->currentMacro != 0;
}
