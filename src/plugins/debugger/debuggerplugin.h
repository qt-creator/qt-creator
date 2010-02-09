/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGERPLUGIN_H
#define DEBUGGERPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QAction;
class QCursor;
class QMenu;
class QPoint;
class QComboBox;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class IMode;
}

namespace TextEditor {
class ITextEditor;
class ITextMark;
class BaseTextMark;
}

namespace Debugger {

class DebuggerManager;
class DebuggerUISwitcher;

namespace Internal {

class BreakpointData;
class DebuggerRunControlFactory;
class DebugMode;

class DebuggerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    struct AttachRemoteParameters {
        AttachRemoteParameters();

        quint64 attachPid;
        QString attachCore;
        // Event handle for attaching to crashed Windows processes.
        quint64 winCrashEvent;
    };

    DebuggerPlugin();
    ~DebuggerPlugin();

private:
    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void shutdown();
    virtual void extensionsInitialized();
    virtual void remoteCommand(const QStringList &options, const QStringList &arguments);

    QVariant configValue(const QString &name) const;

private slots:
    void activatePreviousMode();
    void activateDebugMode();
    void queryCurrentTextEditor(QString *fileName, int *line, QObject **object);
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);
    void handleStateChanged(int state);
    void requestMark(TextEditor::ITextEditor *editor, int lineNumber);
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &pnt, int pos);
    void querySessionValue(const QString &name, QVariant *value);
    void setSessionValue(const QString &name, const QVariant &value);
    void queryConfigValue(const QString &name, QVariant *value);
    void setConfigValue(const QString &name, const QVariant &value);
    void requestContextMenu(TextEditor::ITextEditor *editor,
        int lineNumber, QMenu *menu);

    void resetLocation();
    void gotoLocation(const QString &file, int line, bool setMarker);

    void breakpointSetRemoveMarginActionTriggered();
    void breakpointEnableDisableMarginActionTriggered();
    void onModeChanged(Core::IMode *mode);
    void showSettingsDialog();

    void startExternalApplication();
    void startRemoteApplication();
    void attachExternalApplication();
    void attachCore();
    void attachCmdLine();

private:
    void readSettings();
    void writeSettings() const;
    void attachExternalApplication(qint64 pid, const QString &crashParameter = QString());
    void attachCore(const QString &core, const QString &exeFileName);
    QWidget *createToolbar() const;

    friend class Debugger::DebuggerManager;
    friend class GdbOptionPage;
    friend class DebuggingHelperOptionPage;
    friend class Debugger::Internal::DebugMode; // FIXME: Just a hack now so that it can access the views

    DebuggerUISwitcher *m_uiSwitcher;
    DebuggerManager *m_manager;
    DebugMode *m_debugMode;
    DebuggerRunControlFactory *m_debuggerRunControlFactory;

    QString m_previousMode;
    TextEditor::BaseTextMark *m_locationMark;
    int m_gdbRunningContext;
    AttachRemoteParameters m_attachRemoteParameters;
    unsigned m_cmdLineEnabledEngines;

    QAction *m_startExternalAction;
    QAction *m_startRemoteAction;
    QAction *m_attachExternalAction;
    QAction *m_attachCoreAction;
    QAction *m_detachAction;
    QComboBox *m_langBox;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
