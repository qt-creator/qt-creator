// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "foldernavigationwidget.h"
#include <qglobal.h>

#include <extensionsystem/iplugin.h>
#include <utils/environment.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}

namespace Core {

class FolderNavigationWidgetFactory;
class SessionManager;
class ICore;

namespace Internal {

class EditMode;
class MainWindow;
class Locator;

class CorePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Core.json")

public:
    CorePlugin();
    ~CorePlugin() override;

    static CorePlugin *instance();

    bool initialize(const QStringList &arguments, QString *errorMessage = nullptr) override;
    void extensionsInitialized() override;
    bool delayedInitialize() override;
    ShutdownFlag aboutToShutdown() override;
    QObject *remoteCommand(const QStringList & /* options */,
                           const QString &workingDirectory,
                           const QStringList &args) override;

    static Utils::Environment startupSystemEnvironment();
    static Utils::EnvironmentItems environmentChanges();
    static void setEnvironmentChanges(const Utils::EnvironmentItems &changes);
    static QString msgCrashpadInformation();

public slots:
    void fileOpenRequest(const QString &);

#if defined(WITH_TESTS)
private slots:
    void testVcsManager_data();
    void testVcsManager();
    // Locator:
    void test_basefilefilter();
    void test_basefilefilter_data();

    void testOutputFormatter();
#endif

private:
    static void addToPathChooserContextMenu(Utils::PathChooser *pathChooser, QMenu *menu);
    static void setupSystemEnvironment();
    void checkSettings();
    void warnAboutCrashReporing();

    ICore *m_core = nullptr;
    EditMode *m_editMode = nullptr;
    Locator *m_locator = nullptr;
    std::unique_ptr<SessionManager> m_sessionManager;
    FolderNavigationWidgetFactory *m_folderNavigationWidgetFactory = nullptr;
    Utils::Environment m_startupSystemEnvironment;
    Utils::EnvironmentItems m_environmentChanges;
};

} // namespace Internal
} // namespace Core
