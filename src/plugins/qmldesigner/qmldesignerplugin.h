// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "documentmanager.h"
#include "qmldesigner_global.h"

#include <qmldesignercorelib_global.h>
#include <viewmanager.h>

#include <extensionsystem/iplugin.h>

#include <qmldesignerbase/qmldesignerbaseplugin.h>

#include <QElapsedTimer>

QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QQuickWidget)

namespace Core {
    class IEditor;
}

namespace ADS {
    class DockManager;
}

namespace QmlDesigner {

namespace DeviceShare {
    class DeviceManager;
}

class QmlDesignerPluginPrivate;
class AsynchronousImageCache;
class ExternalDependenciesInterface;
class RunManager;

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
    static DeviceShare::DeviceManager &deviceManager();
    static RunManager &runManager();

    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

    static DesignerSettings &settings();
    static ExternalDependenciesInterface &externalDependenciesForPluginInitializationOnly(); // if you use it your code smells
    static ADS::DockManager *dockManagerForPluginInitializationOnly();

    DesignDocument *currentDesignDocument() const;
    Internal::DesignModeWidget *mainWidget() const;

    static QmlDesignerProjectManager &projectManagerForPluginInitializationOnly();

    QWidget *createProjectExplorerWidget(QWidget *parent) const;

    void switchToTextModeDeferred();
    void sendStatisticsFeedback(const QString &id, const QString &key, int value);

    static double formEditorDevicePixelRatio();

    static void contextHelp(const Core::IContext::HelpCallback &callback, const QString &id);

    static void emitUsageStatistics(const QString &identifier);
    static void emitUsageStatisticsContextAction(const QString &identifier);
    static void emitUsageStatisticsTime(const QString &identifier, int elapsed);
    static void emitUsageStatisticsUsageDuration(const QString &identifier, int elapsed);

    static AsynchronousImageCache &imageCache();

    static void registerPreviewImageProvider(QQmlEngine *engine);

    static void trackWidgetFocusTime(QWidget *widget, const QString &identifier);
    static void registerCombinedTracedPoints(const QString &identifierFirst,
                                             const QString &identifierSecond,
                                             const QString &newIdentifier,
                                             int maxDuration = 10000);

signals:
    void usageStatisticsNotifier(const QString &identifier);
    void usageStatisticsUsageTimer(const QString &identifier, int elapsed);
    void usageStatisticsUsageDuration(const QString &identifier, int elapsed);
    void usageStatisticsInsertFeedback(const QString &identifier,
                                       const QString &feedback,
                                       int rating);

private slots:
    void closeFeedbackPopup();
    void launchFeedbackPopup(const QString &identifier);
    void handleFeedback(const QString &feedback, int rating);

private: // functions
    void launchFeedbackPopupInternal(const QString &identifier);
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
    QString identiferToDisplayString(const QString &identifier);

    RewriterView *rewriterView() const;
    Model *currentModel() const;
    QQuickWidget *m_feedbackWidget = nullptr;
    static QmlDesignerPluginPrivate *privateInstance();
    void enforceDelayedInitialize();

private: // variables
    QmlDesignerPluginPrivate *d = nullptr;
    static QmlDesignerPlugin *m_instance;
    QElapsedTimer m_usageTimer;
    bool m_delayedInitialized = false;
    bool m_shutdownPending = false;
};

} // namespace QmlDesigner
