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

#ifndef DEBUGGERUISWITCHER_H
#define DEBUGGERUISWITCHER_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <QtCore/QObject>
#include <QtCore/QMultiHash>

QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QDockWidget)

namespace Core {
    class ActionContainer;
    class Command;
    class Context;
    class BaseMode;
    class IMode;
}

namespace Utils {
class FancyMainWindow;
    class SavedAction;
}

namespace ProjectExplorer {
    class Project;
    class Target;
    class RunConfiguration;
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
    void addLanguage(const DebuggerLanguage &language, const QString &languageName,
                     const Core::Context &context);

    // debugger toolbars are registered  with this function
    void setToolbar(const DebuggerLanguage &language, QWidget *widget);

    // menu actions are registered with this function
    void addMenuAction(Core::Command *command, const DebuggerLanguage &language,
                       const QString &group = QString());

    // all supported languagess
    DebuggerLanguages supportedLanguages() const;

    // active languages to be debugged.
    DebuggerLanguages activeDebugLanguages() const;

    // called when all dependent plugins have loaded
    void initialize();

    void aboutToShutdown();

    // most common debugger windows
    QDockWidget *breakWindow() const;
    QDockWidget *stackWindow() const;
    QDockWidget *watchWindow() const;
    QDockWidget *snapshotsWindow() const;
    QDockWidget *threadsWindow() const;
    QDockWidget *outputWindow() const;
    QDockWidget *qmlInspectorWindow() const;

    QDockWidget *dockWidget(const QString &objectName) const;

    // dockwidgets are registered to the main window
    QDockWidget *createDockWidget(const DebuggerLanguage &language, QWidget *widget,
                                  Qt::DockWidgetArea area = Qt::TopDockWidgetArea,
                                  bool visibleByDefault = true);

    Utils::FancyMainWindow *mainWindow() const;

signals:
    // emit when user changes active languages from the menu.
    // Both UI and debugger startup are affected.
    void activeLanguagesChanged(Debugger::DebuggerLanguages activeLanguages);

    void dockResetRequested(const Debugger::DebuggerLanguages &activeLanguages);

private slots:
    void modeChanged(Core::IMode *mode);
    void updateUi();
    void resetDebuggerLayout();

    void updateUiForProject(ProjectExplorer::Project *project);
    void updateUiForTarget(ProjectExplorer::Target *target);
    void updateUiForRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void updateUiOnFileListChange();

    void updateActiveLanguages();
    void updateDockWidgetSettings();

    void onModeChanged(Core::IMode *mode);

private:
    // Used by MainWindow
    friend class Internal::DebuggerMainWindow;
    QList<Internal::DebugToolWindow *> i_mw_debugToolWindows() const;

    void activateQmlCppLayout();
    void activateCppLayout();

    void hideInactiveWidgets();
    void createViewsMenuItems();
    void readSettings();
    void writeSettings() const;
    bool isQmlCppActive() const;
    bool isQmlActive() const;

    QWidget *createContents(Core::BaseMode *mode);
    QWidget *createMainWindow(Core::BaseMode *mode);

    DebuggerUISwitcherPrivate *d;
};

namespace Internal {
class DockWidgetEventFilter : public QObject
{
    Q_OBJECT

public:
    DockWidgetEventFilter(QObject *parent = 0);

signals:
    void widgetResized();

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

} // namespace Internal

} // namespace Debugger

#endif // DEBUGGERUISWITCHER_H
