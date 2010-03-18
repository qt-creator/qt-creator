#ifndef DEBUGGERMAINWINDOW_H
#define DEBUGGERMAINWINDOW_H

#include <utils/fancymainwindow.h>

QT_FORWARD_DECLARE_CLASS(QMenu);

namespace Debugger {
class DebuggerUISwitcher;

namespace Internal {

class DebugToolWindow {
public:
    DebugToolWindow() : m_visible(false) {}
    QDockWidget* m_dockWidget;

    int m_languageId;
    bool m_visible;
};

class DebuggerMainWindow : public Utils::FancyMainWindow
{
    Q_OBJECT
public:
    explicit DebuggerMainWindow(DebuggerUISwitcher *uiSwitcher, QWidget *parent = 0);
    virtual ~DebuggerMainWindow();

protected:
    virtual QMenu *createPopupMenu();

private:
    DebuggerUISwitcher *m_uiSwitcher;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERMAINWINDOW_H
