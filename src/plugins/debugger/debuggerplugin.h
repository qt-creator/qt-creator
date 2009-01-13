/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef DEBUGGERPLUGIN_H
#define DEBUGGERPLUGIN_H

#include <projectexplorer/projectexplorer.h>
#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QCursor;
class QAbstractItemView;
QT_END_NAMESPACE

namespace Core { class IEditor; }
namespace TextEditor { class ITextEditor; }

namespace Debugger {
namespace Internal {

class DebuggerManager;
class DebugMode;
class GdbOptionPage;
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

    void resetLocation();
    void gotoLocation(const QString &fileName, int line, bool setMarker);

    void breakpointMarginActionTriggered();
    void focusCurrentEditor(Core::IMode *mode);

private:
    void readSettings();
    void writeSettings() const;

    friend class DebuggerManager;
    friend class DebugMode; // FIXME: Just a hack now so that it can access the views

    ProjectExplorer::ProjectExplorerPlugin *projectExplorer() const;

    DebuggerManager *m_manager;
    DebugMode *m_debugMode;

    ExtensionSystem::PluginManager *m_pm;
    GdbOptionPage *m_generalOptionPage;

    QString m_previousMode;
    LocationMark *m_locationMark;
    int m_gdbRunningContext;

    QAction *m_breakpointMarginAction;
    QAction *m_toggleLockedAction;
    int m_breakpointMarginActionLineNumber;
    QString m_breakpointMarginActionFileName;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERPLUGIN_H
