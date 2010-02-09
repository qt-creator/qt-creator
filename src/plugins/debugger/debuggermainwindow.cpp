#include "debuggermainwindow.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QDockWidget>
#include <QtGui/QStyle>

#include <QDebug>

namespace Debugger {

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

    QList<Internal::DebugToolWindow* > dockwidgets = m_uiSwitcher->m_dockWidgets;

    if (!dockwidgets.isEmpty()) {
        menu = new QMenu(this);

        for (int i = 0; i < dockwidgets.size(); ++i) {
            QDockWidget *dockWidget = dockwidgets.at(i)->m_dockWidget;
            if (dockWidget->parentWidget() == this &&
                dockwidgets.at(i)->m_languageId == m_uiSwitcher->m_activeLanguage) {

                menu->addAction(dockWidget->toggleViewAction());
            }
        }
        menu->addSeparator();
    }

    return menu;
}

}
