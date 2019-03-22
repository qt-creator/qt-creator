/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <QtGlobal>

namespace Debugger {
namespace Constants {

const char DEBUGGER_COMMON_SETTINGS_ID[]   = "A.Debugger.General";
const char DEBUGGER_SETTINGS_CATEGORY[]    = "O.Debugger";

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
    LocalsNameRole,
    LocalsExpandedRole,     // The preferred expanded state to the view
    LocalsTypeFormatRole,   // Used to communicate alternative formats to the view
    LocalsIndividualFormatRole,

    // Snapshots
    SnapshotCapabilityRole
};

} // namespace Debugger
