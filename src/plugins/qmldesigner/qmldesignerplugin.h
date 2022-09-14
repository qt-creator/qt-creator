// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesigner/designersettings.h>
#include <qmldesignercorelib_global.h>

#include <extensionsystem/iplugin.h>

#include "documentmanager.h"
#include "viewmanager.h"
#include "shortcutmanager.h"
#include <designeractionmanager.h>

#include <QElapsedTimer>

QT_FORWARD_DECLARE_CLASS(QQmlEngine)

namespace Core {
    class IEditor;
}

namespace QmlDesigner {

class QmlDesignerPluginPrivate;
class AsynchronousImageCache;

namespace Internal { class DesignModeWidget; }

class QMLDESIGNERCORE_EXPORT QmlDesignerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesigner.json")

public:
    QmlDesignerPlugin();
    ~QmlDesignerPlugin() final;

    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    bool delayedInitialize() final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;

    static QmlDesignerPlugin *instance();

    DocumentManager &documentManager();
    const DocumentManager &documentManager() const;

    ViewManager &viewManager();
    const ViewManager &viewManager() const;

    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

    static DesignerSettings& settings();

    DesignDocument *currentDesignDocument() const;
    Internal::DesignModeWidget *mainWidget() const;

    QWidget *createProjectExplorerWidget(QWidget *parent) const;

    void switchToTextModeDeferred();
    void emitCurrentTextEditorChanged(Core::IEditor *editor);

    static double formEditorDevicePixelRatio();

    static void contextHelp(const Core::IContext::HelpCallback &callback, const QString &id);

    static void emitUsageStatistics(const QString &identifier);
    static void emitUsageStatisticsContextAction(const QString &identifier);
    static void emitUsageStatisticsHelpRequested(const QString &identifier);
    static void emitUsageStatisticsTime(const QString &identifier, int elapsed);

    static AsynchronousImageCache &imageCache();

    static void registerPreviewImageProvider(QQmlEngine *engine);

    static void trackWidgetFocusTime(QWidget *widget, const QString &identifier);

signals:
    void usageStatisticsNotifier(const QString &identifier);
    void usageStatisticsUsageTimer(const QString &identifier, int elapsed);


private: // functions
    void integrateIntoQtCreator(QWidget *modeWidget);
    void showDesigner();
    void hideDesigner();
    void changeEditor();
    void jumpTextCursorToSelectedModelNode();
    void selectModelNodeUnderTextCursor();
    void activateAutoSynchronization();
    void deactivateAutoSynchronization();
    void resetModelSelection();
    RewriterView *rewriterView() const;
    Model *currentModel() const;

private: // variables
    QmlDesignerPluginPrivate *d = nullptr;
    static QmlDesignerPlugin *m_instance;
    QElapsedTimer m_usageTimer;
};

} // namespace QmlDesigner
