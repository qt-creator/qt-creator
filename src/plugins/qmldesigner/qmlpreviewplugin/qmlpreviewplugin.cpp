// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewplugin.h"
#include "qmlpreviewactions.h"

#include <actioninterface.h>
#include <componentcore_constants.h>
#include <designeractionmanager.h>
#include <modelnodecontextmenu_helper.h>
#include <viewmanager.h>
#include <zoomaction.h>
#include <qmldesignerplugin.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

namespace QmlPreview {
using QmlPreviewRunControlList = QList<ProjectExplorer::RunControl *>;
}

Q_DECLARE_METATYPE(QmlPreview::QmlPreviewRunControlList)

namespace QmlDesigner {
static QObject *s_previewPlugin = nullptr;

QmlPreviewWidgetPlugin::QmlPreviewWidgetPlugin()
{
    DesignerActionManager &designerActionManager =
            QmlDesignerPlugin::instance()->designerActionManager();
    auto previewAction = new QmlPreviewAction();
    designerActionManager.addDesignerAction(new ActionGroup(
                                                QString(),
                                                ComponentCoreConstants::qmlPreviewCategory,
                                                {},
                                                ComponentCoreConstants::Priorities::QmlPreviewCategory,
                                                &SelectionContextFunctors::always));
    s_previewPlugin = getPreviewPlugin();

    if (s_previewPlugin) {
        bool connected = connect(s_previewPlugin, SIGNAL(runningPreviewsChanged(const QmlPreviewRunControlList &)),
                this, SLOT(handleRunningPreviews()));
        QTC_ASSERT(connected, qWarning() << "something wrong with the runningPreviewsChanged signal");
    }

    designerActionManager.addDesignerAction(previewAction);

    auto zoomAction = new ZoomPreviewAction;
    designerActionManager.addDesignerAction(zoomAction);

    auto separator = new SeparatorDesignerAction(ComponentCoreConstants::qmlPreviewCategory, 0);
    designerActionManager.addDesignerAction(separator);

    m_previewToggleAction = previewAction->action();

    Core::Context globalContext;
    auto registerCommand = [&globalContext](ActionInterface *action) {
        const QString id = QStringView(u"QmlPreview.%1").arg(QString::fromLatin1(action->menuId()));
        Core::Command *cmd = Core::ActionManager::registerAction(action->action(),
                                                                 id.toLatin1().constData(),
                                                                 globalContext);

        cmd->setDefaultKeySequence(action->action()->shortcut());
        cmd->setDescription(action->action()->toolTip());

        action->action()->setToolTip(cmd->action()->toolTip());
        action->action()->setShortcut(cmd->action()->shortcut());
    };
    // Only register previewAction as others don't have keyboard shortcuts for them
    registerCommand(previewAction);

    if (s_previewPlugin) {
        auto fpsAction = new FpsAction;
        designerActionManager.addDesignerAction(fpsAction);
        bool hasFpsHandler =
            s_previewPlugin->setProperty("fpsHandler", QVariant::fromValue<QmlPreview::QmlPreviewFpsHandler>(FpsLabelAction::fpsHandler));
        QTC_CHECK(hasFpsHandler);
        auto switchLanguageAction = new SwitchLanguageAction;
        designerActionManager.addDesignerAction(switchLanguageAction);
    }
}

QString QmlPreviewWidgetPlugin::pluginName() const
{
    return QLatin1String("QmlPreviewPlugin");
}

void QmlPreviewWidgetPlugin::stopAllRunControls()
{
    QTC_ASSERT(s_previewPlugin, return);

    const QVariant variant = s_previewPlugin->property("runningPreviews");
    auto runControls = variant.value<QmlPreview::QmlPreviewRunControlList>();

    for (ProjectExplorer::RunControl *runControl : runControls)
        runControl->initiateStop();

}

void QmlPreviewWidgetPlugin::handleRunningPreviews()
{
    QTC_ASSERT(s_previewPlugin, return);

    const QVariant variant = s_previewPlugin->property("runningPreviews");
    if (variant.isValid()) {
        // the QmlPreview::QmlPreviewRunControlList type have to be available and used in the qmlpreview plugin
        QTC_ASSERT(variant.canConvert<QmlPreview::QmlPreviewRunControlList>(), return);
        auto runControls = variant.value<QmlPreview::QmlPreviewRunControlList>();
        m_previewToggleAction->setChecked(!runControls.isEmpty());
        if (runControls.isEmpty())
            FpsLabelAction::cleanFpsCounter();
    }
}

QString QmlPreviewWidgetPlugin::metaInfo() const
{
    return QLatin1String(":/qmlpreviewplugin/qmlpreview.metainfo");
}

void QmlPreviewWidgetPlugin::setQmlFile()
{
    if (s_previewPlugin) {
        const Utils::FilePath qmlFileName =
                QmlDesignerPlugin::instance()->currentDesignDocument()->fileName();
        bool hasPreviewedFile =
            s_previewPlugin->setProperty("previewedFile", qmlFileName.toString());
        QTC_CHECK(hasPreviewedFile);
    }
}

float QmlPreviewWidgetPlugin::zoomFactor()
{
    QVariant zoomFactorVariant = 1.0;
    if (s_previewPlugin && !s_previewPlugin->property("zoomFactor").isNull())
        zoomFactorVariant = s_previewPlugin->property("zoomFactor");
    return zoomFactorVariant.toFloat();
}

void QmlPreviewWidgetPlugin::setZoomFactor(float zoomFactor)
{
    if (auto s_previewPlugin = getPreviewPlugin()) {
        bool hasZoomFactor = s_previewPlugin->setProperty("zoomFactor", zoomFactor);
        QTC_CHECK(hasZoomFactor);
    }
}

void QmlPreviewWidgetPlugin::setLanguageLocale(const QString &locale)
{
    if (auto s_previewPlugin = getPreviewPlugin()) {
        bool hasLocaleIsoCode = s_previewPlugin->setProperty("localeIsoCode", locale);
        QTC_CHECK(hasLocaleIsoCode);
    }
}

QObject *QmlPreviewWidgetPlugin::getPreviewPlugin()
{
    const QVector<ExtensionSystem::PluginSpec *> &specs = ExtensionSystem::PluginManager::plugins();
    const auto pluginIt = std::find_if(specs.cbegin(), specs.cend(),
                                 [](const ExtensionSystem::PluginSpec *p) {
        return p->name() == "QmlPreview";
    });

    if (pluginIt != specs.cend())
        return (*pluginIt)->plugin();

    return nullptr;
}

} // namespace QmlDesigner
