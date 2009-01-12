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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "icore.h"

#include <QtGui/QMainWindow>
#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QPointer>
#include <QtGui/QPrinter>
#include <QtGui/QToolButton>

QT_BEGIN_NAMESPACE
class QSettings;
class QShortcut;
QT_END_NAMESPACE

namespace ExtensionSystem {
class PluginManager;
}

namespace Core {

class ActionManagerInterface;
class BaseMode;
class BaseView;
class EditorManager;
class FileManager;
class IContext;
class MessageManager;
class MimeDatabase;
class ModeManager;
class ProgressManagerInterface;
class RightPaneWidget;
class ScriptManagerInterface;
class UniqueIDManager;
class VariableManager;
class VCSManager;
class ViewManagerInterface;

namespace Internal {

class ActionManager;
class CoreImpl;
class FancyTabWidget;
class GeneralSettings;
class NavigationWidget;
class OutputPane;
class ProgressManager;
class ShortcutSettings;
class ViewManager;
class VersionDialog;

class CORE_EXPORT  MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    bool init(ExtensionSystem::PluginManager *pm, QString *error_message);
    void extensionsInitialized();

    IContext *contextObject(QWidget *widget);
    void addContextObject(IContext *contex);
    void removeContextObject(IContext *contex);
    void resetContext();

    void openFiles(const QStringList &fileNames);

    //ICore
    inline ExtensionSystem::PluginManager *pluginManager() { return m_pluginManager; }
    Core::ActionManagerInterface *actionManager() const;
    Core::FileManager *fileManager() const;
    Core::UniqueIDManager *uniqueIDManager() const;
    Core::MessageManager *messageManager() const;
    ExtensionSystem::PluginManager *pluginManager() const;
    Core::EditorManager *editorManager() const;
    Core::ProgressManagerInterface *progressManager() const;
    Core::ScriptManagerInterface *scriptManager() const;
    Core::VariableManager *variableManager() const;
    Core::ModeManager *modeManager() const;
    Core::MimeDatabase *mimeDatabase() const;

    VCSManager *vcsManager() const;
    inline QSettings *settings() const { return m_settings; }
    virtual QPrinter *printer() const;
    IContext * currentContextObject() const;
    QStatusBar *statusBar() const;
    void addAdditionalContext(int context);
    void removeAdditionalContext(int context);
    bool hasContext(int context) const;

    void updateContext();

    QMenu *createPopupMenu();

    void setSuppressNavigationWidget(bool suppress);

signals:
    void windowActivated();

public slots:
    void newFile();
    void openFileWith();
    void exit();

    QStringList showNewItemDialog(const QString &title,
                                  const QList<IWizard *> &wizards,
                                  const QString &defaultLocation = QString());

    void showOptionsDialog(const QString &category = QString(), const QString &page = QString());

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);

private slots:
    void openFile();
    void aboutToShowRecentFiles();
    void openRecentFile();
    void setFocusToEditor();
    void saveAll();
    void aboutQtCreator();
    void aboutPlugins();
    void updateFocusWidget(QWidget *old, QWidget *now);
    void toggleNavigation();
    void destroyVersionDialog();

private:
    void updateContextObject(IContext *context);
    void registerDefaultContainers();
    void registerDefaultActions();

    void readSettings();
    void writeSettings();

    CoreImpl *m_coreImpl;
    UniqueIDManager *m_uniqueIDManager;
    QList<int> m_globalContext;
    QList<int> m_additionalContexts;
    QSettings *m_settings;
    mutable QPrinter *m_printer;
    ActionManager *m_actionManager;
    EditorManager *m_editorManager;
    FileManager *m_fileManager;
    MessageManager *m_messageManager;
    ProgressManager *m_progressManager;
    ScriptManagerInterface *m_scriptManager;
    VariableManager *m_variableManager;
    VCSManager *m_vcsManager;
    ViewManager *m_viewManager;
    ModeManager *m_modeManager;
    MimeDatabase *m_mimeDatabase;
    FancyTabWidget *m_modeStack;
    NavigationWidget *m_navigationWidget;
    RightPaneWidget *m_rightPaneWidget;
    Core::BaseView *m_outputView;
    VersionDialog *m_versionDialog;

    IContext * m_activeContext;

    QMap<QWidget *, IContext *> m_contextWidgets;

    ExtensionSystem::PluginManager *m_pluginManager;

    OutputPane *m_outputPane;
    BaseMode *m_outputMode;
    GeneralSettings *m_generalSettings;
    ShortcutSettings *m_shortcutSettings;

    QMap<QAction*, QString> m_recentFilesActions;

    // actions
    QShortcut *m_focusToEditor;
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_openWithAction;
    QAction *m_saveAllAction;
    QAction *m_exitAction;
    QAction *m_optionsAction;
    QAction *m_toggleSideBarAction;
#ifdef Q_OS_MAC
    QAction *m_minimizeAction;
    QAction *m_zoomAction;
#endif

    QToolButton *m_toggleSideBarButton;
};

} // namespace Internal
} // namespace Core

#endif // MAINWINDOW_H
