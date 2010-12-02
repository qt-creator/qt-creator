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

#include <utils/fancymainwindow.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QDockWidget;
class QSettings;
class QMenu;
QT_END_NAMESPACE

namespace Core {
class Context;
class IMode;
}

namespace Debugger {

namespace Internal {
class DebuggerMainWindowPrivate;
}

class DEBUGGER_EXPORT DebuggerMainWindow : public Utils::FancyMainWindow
{
    Q_OBJECT

public:
    DebuggerMainWindow();
    ~DebuggerMainWindow();

    // Debuggable languages are registered with this function.
    void addLanguage(const DebuggerLanguage &language, const Core::Context &context);

    // Debugger toolbars are registered  with this function
    void setToolbar(const DebuggerLanguage &language, QWidget *widget);

    // All supported languages
    DebuggerLanguages supportedLanguages() const;

    // Active languages to be debugged.
    DebuggerLanguages activeDebugLanguages() const;

    // Called when all dependent plugins have loaded
    void initialize(QSettings *settings);

    void onModeChanged(Core::IMode *mode);

    // Most common debugger windows
    QDockWidget *breakWindow() const;
    QDockWidget *stackWindow() const;
    QDockWidget *watchWindow() const;
    QDockWidget *snapshotsWindow() const;
    QDockWidget *threadsWindow() const;
    QDockWidget *outputWindow() const;
    QDockWidget *qmlInspectorWindow() const;

    QDockWidget *dockWidget(const QString &objectName) const;

    // Dockwidgets are registered to the main window.
    QDockWidget *createDockWidget(const DebuggerLanguage &language, QWidget *widget,
                                  Qt::DockWidgetArea area = Qt::TopDockWidgetArea);

    QWidget *createContents(Core::IMode *mode);
    QMenu *createPopupMenu();

signals:
    // Emit when user changes active languages from the menu.
    // Both UI and debugger startup are affected.
    void activeLanguagesChanged(Debugger::DebuggerLanguages activeLanguages);
    void dockResetRequested(Debugger::DebuggerLanguages activeLanguages);
    void memoryEditorRequested();

public slots:
    void updateActiveLanguages();
    void updateDockWidgetSettings();
    void readSettings();
    void writeSettings() const;

private:
    friend class Internal::DebuggerMainWindowPrivate;
    Internal::DebuggerMainWindowPrivate *d;
};

} // namespace Debugger

#endif // DEBUGGERUISWITCHER_H
