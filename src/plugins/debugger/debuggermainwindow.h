/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef DEBUGGERUISWITCHER_H
#define DEBUGGERUISWITCHER_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <utils/fancymainwindow.h>

namespace Core {
class Context;
class IMode;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {
class DebuggerMainWindowPrivate;
}

class DEBUGGER_EXPORT DebuggerMainWindow : public Utils::FancyMainWindow
{
    Q_OBJECT

public:
    DebuggerMainWindow();
    ~DebuggerMainWindow();

    void setCurrentEngine(DebuggerEngine *engine);

    // Debugger toolbars are registered with this function.
    void setToolBar(DebuggerLanguage language, QWidget *widget);

    // Active languages to be debugged.
    DebuggerLanguages activeDebugLanguages() const;
    void setEngineDebugLanguages(DebuggerLanguages languages);

    // Called when all dependent plugins have loaded.
    void initialize();

    void onModeChanged(Core::IMode *mode);
    QDockWidget *dockWidget(const QString &objectName) const;
    bool isDockVisible(const QString &objectName) const;

    // Dockwidgets are registered to the main window.
    QDockWidget *createDockWidget(const DebuggerLanguage &language, QWidget *widget);
    void addStagedMenuEntries();

    QWidget *createContents(Core::IMode *mode);
    QMenu *createPopupMenu();

    void readSettings();
    void writeSettings() const;

private slots:
    void raiseDebuggerWindow();

private:
    friend class Internal::DebuggerMainWindowPrivate;
    Internal::DebuggerMainWindowPrivate *d;
};

} // namespace Debugger

#endif // DEBUGGERUISWITCHER_H
