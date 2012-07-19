/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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

signals:
    void activeDebugLanguagesChanged(Debugger::DebuggerLanguages);

private:
    friend class Internal::DebuggerMainWindowPrivate;
    Internal::DebuggerMainWindowPrivate *d;
};

} // namespace Debugger

#endif // DEBUGGERUISWITCHER_H
