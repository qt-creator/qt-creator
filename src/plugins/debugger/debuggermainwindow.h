#ifndef DEBUGGERMAINWINDOW_H
#define DEBUGGERMAINWINDOW_H

#include "debuggeruiswitcher.h"
#include <utils/fancymainwindow.h>


class QMenu;

namespace Debugger {

class DebuggerMainWindow : public Utils::FancyMainWindow
{
public:
    DebuggerMainWindow(DebuggerUISwitcher *uiSwitcher, QWidget *parent = 0);
    ~DebuggerMainWindow();


protected:
    virtual QMenu *createPopupMenu();

private:
    DebuggerUISwitcher *m_uiSwitcher;
};

}

#endif // DEBUGGERMAINWINDOW_H
