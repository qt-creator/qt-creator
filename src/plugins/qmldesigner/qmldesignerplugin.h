// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "documentmanager.h"
#include "qmldesigner_global.h"

#include <qmldesignercorelib_global.h>
#include <viewmanager.h>

#include <extensionsystem/iplugin.h>

#include <qmldesigner/qmldesignerplugin.h>


QT_BEGIN_NAMESPACE
class QStyle;
class QQmlEngine;
class QQuickWidget;
QT_END_NAMESPACE

namespace Core {
    class IEditor;
}

namespace ADS {
    class DockManager;
}

namespace QmlDesigner {

class QmlDesignerPluginPrivate;
class AsynchronousImageCache;
class ExternalDependenciesInterface;

namespace Internal { class DesignModeWidget; }


class QMLDESIGNER_EXPORT QmlDesignerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesigner.json")

public:
    QmlDesignerPlugin();
    ~QmlDesignerPlugin() final;

    Utils::Result<> initialize(const QStringList &arguments) final;
    bool delayedInitialize() final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;

    static QmlDesignerPlugin *instance();

    DocumentManager &documentManager();
    const DocumentManager &documentManager() const;

    static ViewManager &viewManager();

    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

    static ExternalDependenciesInterface &externalDependenciesForPluginInitializationOnly(); // if you use it your code smells
    static ADS::DockManager *dockManagerForPluginInitializationOnly();

    DesignDocument *currentDesignDocument() const;
    Internal::DesignModeWidget *mainWidget() const;

    void switchToTextModeDeferred();

    static double formEditorDevicePixelRatio();

    static void contextHelp(const Core::IContext::HelpCallback &callback, const QString &id);

    static AsynchronousImageCache &imageCache();

private: // functions
    void integrateIntoQtCreator(Internal::DesignModeWidget *modeWidget);
    void clearDesigner();
    void resetDesignerDocument();
    void setupDesigner();
    void showDesigner();
    void hideDesigner();
    void changeEditor();
    void jumpTextCursorToSelectedModelNode();
    void selectModelNodeUnderTextCursor();
    void activateAutoSynchronization();
    void deactivateAutoSynchronization();
    void resetModelSelection();
    void initializeShutdownSettings();
    RewriterView *rewriterView() const;
    Model *currentModel() const;
    static QmlDesignerPluginPrivate *privateInstance();
    void enforceDelayedInitialize();

private: // variables
    QmlDesignerPluginPrivate *d = nullptr;
    static QmlDesignerPlugin *m_instance;
    bool m_delayedInitialized = false;
    bool m_shutdownPending = false;
};

} // namespace QmlDesigner
