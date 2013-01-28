/**************************************************************************
**
** Copyright (c) 2013 Nicolas Arnaud-Cormos
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "imacrohandler.h"

#include "macro.h"

using namespace Macros;

/*!
    \class Macro::IEventHandler
    \brief Base class for all macro event handlers.

    An event handler is used to handle a specific type of macro events.
    They are used to create and replay macro events, use MacroManager::registerEventHandler
    to add a new event handler.
*/

/*!
    \fn void IEventHandler::startRecording(Macro* macro)

    This method is called when starting to record a macro, it can be used
    to initialize some data.
*/

/*!
    \fn void IEventHandler::endRecordingMacro(Macro* macro)

    This method is called after recording a macro, to cleanup everything.
*/

/*!
    \fn bool IEventHandler::canExecuteEvent(const MacroEvent &macroEvent)

    When replaying a macro, the manager iterate through all macro events
    in the macro and call this method to know which handler to use.
    If the method returns true, executeEvent is called.
*/

/*!
    \fn bool IEventHandler::executeEvent(const MacroEvent &macroEvent)

    This method execute a specific macro event, using the values stored in
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
