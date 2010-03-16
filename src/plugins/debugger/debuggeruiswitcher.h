#ifndef DEBUGGERUISWITCHER_H
#define DEBUGGERUISWITCHER_H

#include "debugger_global.h"

#include <coreplugin/basemode.h>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMap>

namespace Core {
    class ActionContainer;
    class Command;
}

QT_FORWARD_DECLARE_CLASS(Action);
QT_FORWARD_DECLARE_CLASS(QDockWidget);
QT_FORWARD_DECLARE_CLASS(QStackedWidget);
QT_FORWARD_DECLARE_CLASS(QComboBox);
QT_FORWARD_DECLARE_CLASS(QActionGroup);

namespace Debugger {
    class DebuggerMainWindow;
namespace Internal {
class DebugToolWindow {
public:
    DebugToolWindow() : m_visible(false) {}
    QDockWidget* m_dockWidget;
    int m_languageId;
    bool m_visible;
};
}
}

namespace Debugger {
class DEBUGGER_EXPORT DebuggerUISwitcher : public QObject
{
    Q_OBJECT
public:
    DebuggerUISwitcher(Core::BaseMode *mode, QObject *parent = 0);
    ~DebuggerUISwitcher();

    static DebuggerUISwitcher *instance();

    // debuggable languages are registered with this function
    void addLanguage(const QString &langName);

    // debugger toolbars are registered  with this function
    void setToolbar(const QString &langName, QWidget *widget);

    // menu actions are registered with this function
    void addMenuAction(Core::Command *command,
                            const QString &group = QString());

    void setActiveLanguage(const QString &langName);

    // called when all dependent plugins have loaded
    void initialize();

    void shutdown();

    // dockwidgets are registered to the main window
    QDockWidget *createDockWidget(const QString &langName, QWidget *widget,
                                  Qt::DockWidgetArea area = Qt::TopDockWidgetArea,
                                  bool visibleByDefault = true);

    DebuggerMainWindow *mainWindow() const;

signals:
    void languageChanged(const QString &langName);
    // emit when dock needs to be reset
    void dockArranged(const QString &activeLanguage);

private slots:
    void modeChanged(Core::IMode *mode);
    void changeDebuggerUI(const QString &langName);
    void resetDebuggerLayout();
    void langChangeTriggered();

private:
    void hideInactiveWidgets();
    void createViewsMenuItems();
    void readSettings();
    void writeSettings() const;
    QWidget *createContents(Core::BaseMode *mode);
    QWidget *createMainWindow(Core::BaseMode *mode);

    // first: language id, second: menu item
    typedef QPair<int, QAction* > ViewsMenuItems;
    QList< ViewsMenuItems > m_viewsMenuItems;
    QList< Internal::DebugToolWindow* > m_dockWidgets;

    QMap<QString, QWidget *> m_toolBars;
    QStringList m_languages;

    QStackedWidget *m_toolbarStack;
    DebuggerMainWindow *m_mainWindow;

    QList<int> m_debuggercontext;
    QActionGroup *m_languageActionGroup;

    int m_activeLanguage;
    bool m_isActiveMode;
    bool m_changingUI;

    QAction *m_toggleLockedAction;

    const static int StackIndexRole = Qt::UserRole + 11;

    Core::ActionContainer *m_languageMenu;
    Core::ActionContainer *m_viewsMenu;
    Core::ActionContainer *m_debugMenu;

    static DebuggerUISwitcher *m_instance;

    friend class DebuggerMainWindow;
};

}

#endif // DEBUGGERUISWITCHER_H
