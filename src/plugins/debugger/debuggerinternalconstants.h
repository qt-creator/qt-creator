/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef DEBUGGERINTERNALCONSTANTS_H
#define DEBUGGERINTERNALCONSTANTS_H

#include <QtGlobal>

namespace Debugger {
namespace Constants {

const char DEBUGGER_COMMON_SETTINGS_ID[]   = "A.Common";
const char DEBUGGER_SETTINGS_CATEGORY[]    = "O.Debugger";
const char DEBUGGER_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Debugger", "Debugger");
const char DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON[] = ":/debugger/images/category_debug.png";

namespace Internal {
    enum { debug = 0 };
} // namespace Internal

const char OPENED_BY_DEBUGGER[]         = "OpenedByDebugger";
const char OPENED_WITH_DISASSEMBLY[]    = "DisassemblerView";
const char DISASSEMBLER_SOURCE_FILE[]   = "DisassemblerSourceFile";

// Debug action
const char DEBUG[]                      = "Debugger.Debug";
const int  P_ACTION_DEBUG               = 90; // Priority for the modemanager.

} // namespace Constants

enum ModelRoles
{
    DisplaySourceRole = 32,  // Qt::UserRole

    EngineStateRole,
    EngineActionsEnabledRole,
    RequestActivationRole,
    RequestContextMenuRole,

    // Locals and Watchers
    LocalsINameRole,
    LocalsEditTypeRole,     // A QVariant::type describing the item
    LocalsIntegerBaseRole,  // Number base 16, 10, 8, 2
    LocalsNameRole,
    LocalsExpressionRole,
    LocalsRawExpressionRole,
    LocalsExpandedRole,     // The preferred expanded state to the view
    LocalsRawTypeRole,      // Raw type name
    LocalsTypeRole,         // Display type name
    LocalsTypeFormatListRole,
    LocalsTypeFormatRole,   // Used to communicate alternative formats to the view
    LocalsIndividualFormatRole,
    LocalsObjectAddressRole,      // Memory address of variable as quint64
    LocalsSizeRole,         // Size of variable as quint
    LocalsRawValueRole,     // Unformatted value as string
    LocalsPointerAddressRole, // Address of (undereferenced) pointer as quint64
    LocalsIsWatchpointAtObjectAddressRole,
    LocalsIsWatchpointAtPointerAddressRole,

    // Snapshots
    SnapshotCapabilityRole
};

} // namespace Debugger

#endif // DEBUGGERINTERNALCONSTANTS_H

