// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "foldernavigationwidget.h"
#include <qglobal.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginspec.h>

namespace Core {

class FolderNavigationWidgetFactory;
class ICore;

namespace Internal {

class EditMode;
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

    static QString msgCrashpadInformation();

    static void loadMimeFromPlugin(const ExtensionSystem::PluginSpec *plugin);

public slots:
    void fileOpenRequest(const QString &);

#if defined(WITH_TESTS)
private slots:
    // Locator:
    void test_basefilefilter();
    void test_basefilefilter_data();

    void testOutputFormatter();
#endif

private:
    void checkSettings();
    void warnAboutCrashReporing();

    ICore *m_core = nullptr;
    EditMode *m_editMode = nullptr;
    Locator *m_locator = nullptr;
    FolderNavigationWidgetFactory *m_folderNavigationWidgetFactory = nullptr;
};

} // namespace Internal
} // namespace Core
