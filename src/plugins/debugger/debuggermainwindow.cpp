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

#include "debuggermainwindow.h"
#include "debuggeruiswitcher.h"

#include <QtGui/QMenu>
#include <QtGui/QDockWidget>

#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

DebuggerMainWindow::DebuggerMainWindow(DebuggerUISwitcher *uiSwitcher, QWidget *parent) :
        FancyMainWindow(parent), m_uiSwitcher(uiSwitcher)
{
    // TODO how to "append" style sheet?
    // QString sheet;
    // After setting it, all prev. style stuff seem to be ignored.
    /* sheet = QLatin1String(
            "Debugger--DebuggerMainWindow::separator {"
            "    background: black;"
            "    width: 1px;"
            "    height: 1px;"
            "}"
            );
    setStyleSheet(sheet);
    */
}

DebuggerMainWindow::~DebuggerMainWindow()
{

}

QMenu* DebuggerMainWindow::createPopupMenu()
{
    QMenu *menu = 0;

    const QList<Internal::DebugToolWindow* > dockwidgets = m_uiSwitcher->i_mw_debugToolWindows();

    if (!dockwidgets.isEmpty()) {
        menu = new QMenu(this);

        for (int i = 0; i < dockwidgets.size(); ++i) {
            QDockWidget *dockWidget = dockwidgets.at(i)->m_dockWidget;
            if (dockWidget->parentWidget() == this &&
                dockwidgets.at(i)->m_languageId == m_uiSwitcher->activeLanguageId()) {

                menu->addAction(dockWidget->toggleViewAction());
            }
        }
        menu->addSeparator();
    }

    return menu;
}

} // namespace Internal
} // namespace Debugger
