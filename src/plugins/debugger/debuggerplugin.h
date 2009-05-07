/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGERPLUGIN_H
#define DEBUGGERPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QAction;
class QCursor;
class QMenu;
class QPoint;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class IMode;
}

namespace TextEditor {
class ITextEditor;
}

namespace Debugger {
namespace Internal {

class BreakpointData;
class DebuggerManager;
class DebuggerRunner;
class DebugMode;
class LocationMark;

class DebuggerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    DebuggerPlugin();
    ~DebuggerPlugin();

private:
    bool initialize(const QStringList &arguments, QString *error_message);
    void shutdown();
    void extensionsInitialized();

private slots:
    void activatePreviousMode();
    void activateDebugMode();
    void queryCurrentTextEditor(QString *fileName, int *line, QObject **object);
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);
    void changeStatus(int status);
    void requestMark(TextEditor::ITextEditor *editor, int lineNumber);
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &pnt, int pos);
    void querySessionValue(const QString &name, QVariant *value);
    void setSessionValue(const QString &name, const QVariant &value);
    void queryConfigValue(const QString &name, QVariant *value);
    void setConfigValue(const QString &name, const QVariant &value);
    void requestContextMenu(TextEditor::ITextEditor *editor,
        int lineNumber, QMenu *menu);
    void updateActions(int status);


    void resetLocation();
    void gotoLocation(const QString &fileName, int line, bool setMarker);

    void breakpointSetRemoveMarginActionTriggered();
    void breakpointEnableDisableMarginActionTriggered();
    void focusCurrentEditor(Core::IMode *mode);
    void showSettingsDialog();

    void startExternalApplication();
    void startRemoteApplication();
    void attachExternalApplication();
    void attachCore();
    void attachRemoteTcf();

private:
    void readSettings();
    void writeSettings() const;

    friend class DebuggerManager;
    friend class GdbOptionPage;
    friend class DebuggingHelperOptionPage;
    friend class DebugMode; // FIXME: Just a hack now so that it can access the views

    DebuggerManager *m_manager;
    DebugMode *m_debugMode;
    DebuggerRunner *m_debuggerRunner;

    QString m_previousMode;
    LocationMark *m_locationMark;
    int m_gdbRunningContext;

    QAction *m_toggleLockedAction;

    QAction *m_startExternalAction;
    QAction *m_startRemoteAction;
    QAction *m_attachExternalAction;
    QAction *m_attachCoreAction;
    QAction *m_detachAction;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
