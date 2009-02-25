/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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

const char * const M_DEBUG_VIEWS        = "Debugger.Menu.View.Debug";

const char * const C_GDBDEBUGGER        = "Gdb Debugger";
const char * const GDBRUNNING           = "Gdb.Running";

const char * const PROPERTY_REGISTER_FORMAT = "Debugger.Property.RegisterFormat";

} // namespace Constants
} // namespace Debugger

#endif // DEBUGGERCONSTANTS_H

