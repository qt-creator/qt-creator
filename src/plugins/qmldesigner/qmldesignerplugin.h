// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "documentmanager.h"
#include "qmldesigner_global.h"

#include <designersettings.h>
#include <viewmanager.h>
#include <qmldesignercorelib_global.h>

#include <extensionsystem/iplugin.h>

#include <qmldesignerbase/qmldesignerbaseplugin.h>

#include <QElapsedTimer>

QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QQuickWidget)

namespace Core {
    class IEditor;
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

    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    bool delayedInitialize() final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;

    static QmlDesignerPlugin *instance();

    DocumentManager &documentManager();
    const DocumentManager &documentManager() const;

    static ViewManager &viewManager();

    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

    static DesignerSettings &settings();
    static ExternalDependenciesInterface &externalDependenciesForPluginInitializationOnly(); // if you use it your code smells

    DesignDocument *currentDesignDocument() const;
    Internal::DesignModeWidget *mainWidget() const;

    QWidget *createProjectExplorerWidget(QWidget *parent) const;

    void switchToTextModeDeferred();
    void emitCurrentTextEditorChanged(Core::IEditor *editor);

    void emitAssetChanged(const QString &assetPath);

    static double formEditorDevicePixelRatio();

    static void contextHelp(const Core::IContext::HelpCallback &callback, const QString &id);

    static void emitUsageStatistics(const QString &identifier);
    static void emitUsageStatisticsContextAction(const QString &identifier);
    static void emitUsageStatisticsHelpRequested(const QString &identifier);
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
    void assetChanged(const QString &assetPath);

private slots:
    void closeFeedbackPopup();
    void lauchFeedbackPopup(const QString &identifier);
    void handleFeedback(const QString &feedback, int rating);

private: // functions
    void lauchFeedbackPopupInternal(const QString &identifier);
    void integrateIntoQtCreator(QWidget *modeWidget);
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
};

} // namespace QmlDesigner
