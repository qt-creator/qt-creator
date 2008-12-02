/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#ifndef DEBUGGERCONSTANTS_H
#define DEBUGGERCONSTANTS_H

namespace Debugger {
namespace Constants {

// modes and their priorities
const char * const MODE_DEBUG           = "Debugger.Mode.Debug";
const int          P_MODE_DEBUG         = 85;

// common actions
const char * const INTERRUPT            = "Debugger.Interrupt";
const char * const RESET                = "Debugger.Reset";
const char * const STEP                 = "Debugger.StepLine";
const char * const STEPOUT              = "Debugger.StepOut";
const char * const NEXT                 = "Debugger.NextLine";
const char * const STEPI                = "Debugger.StepInstruction";
const char * const NEXTI                = "Debugger.NextInstruction";

const char * const M_VIEW_DEBUG         = "Debugger.Menu.View.Debug";
const char * const G_DEBUG              = "Debugger.Group.Debug";
const char * const G_VIEW_DEBUG         = "Debugger.Group.View.Debug";

const char * const C_GDBDEBUGGER        = "Gdb Debugger";
const char * const GDBRUNNING           = "Gdb.Running";

const char * const PROPERTY_REGISTER_FORMAT = "Debugger.Property.RegisterFormat";

} // namespace Constants
} // namespace Debugger

#endif // DEBUGGERCONSTANTS_H

