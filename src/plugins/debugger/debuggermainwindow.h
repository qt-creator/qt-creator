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

#ifndef DEBUGGERMAINWINDOW_H
#define DEBUGGERMAINWINDOW_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <utils/fancymainwindow.h>

namespace Core { class IMode; }

namespace Debugger {
namespace Internal {

class DebuggerMainWindowPrivate;

// DebuggerMainWindow dock widget names
const char DOCKWIDGET_BREAK[]         = "Debugger.Docks.Break";
const char DOCKWIDGET_MODULES[]       = "Debugger.Docks.Modules";
const char DOCKWIDGET_REGISTER[]      = "Debugger.Docks.Register";
const char DOCKWIDGET_OUTPUT[]        = "Debugger.Docks.Output";
const char DOCKWIDGET_SNAPSHOTS[]     = "Debugger.Docks.Snapshots";
const char DOCKWIDGET_STACK[]         = "Debugger.Docks.Stack";
const char DOCKWIDGET_SOURCE_FILES[]  = "Debugger.Docks.SourceFiles";
const char DOCKWIDGET_THREADS[]       = "Debugger.Docks.Threads";
const char DOCKWIDGET_WATCHERS[]      = "Debugger.Docks.LocalsAndWatchers";
const char DOCKWIDGET_QML_INSPECTOR[] = "Debugger.Docks.QmlInspector";
const char DOCKWIDGET_DEFAULT_AREA[]  = "Debugger.Docks.DefaultArea";
} // namespace Internal

class DEBUGGER_EXPORT DebuggerMainWindow : public Utils::FancyMainWindow
{
    Q_OBJECT

public:
    DebuggerMainWindow();
    ~DebuggerMainWindow();

    // Debugger toolbars are registered with this function.
    void setToolBar(DebuggerLanguage language, QWidget *widget);

    // Active languages to be debugged.
    DebuggerLanguages activeDebugLanguages() const;
    void setEngineDebugLanguages(DebuggerLanguages languages);

    // Called when all dependent plugins have loaded.
    void initialize();

    void onModeChanged(Core::IMode *mode);

    // Dockwidgets are registered to the main window.
    QDockWidget *createDockWidget(const DebuggerLanguage &language, QWidget *widget);
    void addStagedMenuEntries();

    QWidget *createContents(Core::IMode *mode);

    void readSettings();
    void writeSettings() const;

private:
    friend class Internal::DebuggerMainWindowPrivate;
    Internal::DebuggerMainWindowPrivate *d;
};

} // namespace Debugger

#endif // DEBUGGERMAINWINDOW_H
