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

#ifndef CDBBREAKPOINTS_H
#define CDBBREAKPOINTS_H

#include "cdbcom.h"

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {
class BreakpointData;
class BreakpointResponse;

// Convert breakpoint structs
bool addCdbBreakpoint(CIDebugControl* debugControl,
                      CIDebugSymbols *syms,
                      const BreakpointData *nbd,
                      BreakpointResponse *response,
                      QString *errorMessage);

bool deleteCdbBreakpoints(CIDebugControl* debugControl, QString *errorMessage);

void debugCdbBreakpoints(CIDebugControl* debugControl);

// Set response from data.
void setBreakpointResponse(const BreakpointData *nbd, int number, BreakpointResponse *response);

} // namespace Internal
} // namespace Debugger

#endif // CDBBREAKPOINTS_H
