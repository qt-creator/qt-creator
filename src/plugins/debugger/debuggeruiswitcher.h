#ifndef DEBUGGERUISWITCHER_H
#define DEBUGGERUISWITCHER_H

#include "debugger_global.h"

#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QDockWidget);
#include <QtCore/QMultiHash>

namespace Core {
    class ActionContainer;
    class Command;
    class BaseMode;
    class IMode;
}

namespace Utils {
class FancyMainWindow;
    class SavedAction;
}


namespace Debugger {
struct DebuggerUISwitcherPrivate;

namespace Internal {
    class DebugToolWindow;
    class DebuggerMainWindow;
};

class DEBUGGER_EXPORT DebuggerUISwitcher : public QObject
{
    Q_OBJECT
public:
    explicit DebuggerUISwitcher(Core::BaseMode *mode, QObject *parent = 0);
    virtual ~DebuggerUISwitcher();

    static DebuggerUISwitcher *instance();

    // debuggable languages are registered with this function.
    void addLanguage(const QString &langName, const QList<int> &context);

    // debugger toolbars are registered  with this function
    void setToolbar(const QString &langName, QWidget *widget);

    // menu actions are registered with this function
    void addMenuAction(Core::Command *command, const QString &langName,
                       const QString &group = QString());

    // Changes the active language UI to the one specified by langName.
    // Does nothing if automatic switching is toggled off from settings.
    void setActiveLanguage(const QString &langName);
    int activeLanguageId() const;

    // called when all dependent plugins have loaded
    void initialize();

    void shutdown();

    // dockwidgets are registered to the main window
    QDockWidget *createDockWidget(const QString &langName, QWidget *widget,
                                  Qt::DockWidgetArea area = Qt::TopDockWidgetArea,
                                  bool visibleByDefault = true);

    Utils::FancyMainWindow *mainWindow() const;

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
    // Used by MainWindow
    friend class Internal::DebuggerMainWindow;
    QList<Internal::DebugToolWindow* > i_mw_debugToolWindows() const;

    void hideInactiveWidgets();
    void createViewsMenuItems();
    void readSettings();
    void writeSettings() const;
    QWidget *createContents(Core::BaseMode *mode);
    QWidget *createMainWindow(Core::BaseMode *mode);

    DebuggerUISwitcherPrivate *d;
    Utils::SavedAction *m_changeLanguageAction;
};

}

#endif // DEBUGGERUISWITCHER_H
