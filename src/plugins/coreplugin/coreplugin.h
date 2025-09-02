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

class CorePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Core.json")

public:
    CorePlugin();
    ~CorePlugin() final;

    static CorePlugin *instance();

    Utils::Result<> initialize(const QStringList &arguments) final;
    void extensionsInitialized() final;
    bool delayedInitialize() final;
    ShutdownFlag aboutToShutdown() final;
    QObject *remoteCommand(const QStringList & /* options */,
                           const QString &workingDirectory,
                           const QStringList &args) final;

    static QString msgCrashpadInformation();

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
