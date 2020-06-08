/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlpreviewplugin.h"
#include "qmlpreviewactions.h"

#include <modelnodecontextmenu_helper.h>
#include <componentcore_constants.h>
#include <qmldesignerplugin.h>
#include <viewmanager.h>
#include <actioninterface.h>
#include <zoomaction.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

namespace QmlPreview {
using QmlPreviewRunControlList = QList<ProjectExplorer::RunControl *>;
}

Q_DECLARE_METATYPE(QmlPreview::QmlPreviewRunControlList)

namespace QmlDesigner {
static QObject *s_previewPlugin = nullptr;

QmlPreviewPlugin::QmlPreviewPlugin()
{
    DesignerActionManager &designerActionManager =
            QmlDesignerPlugin::instance()->designerActionManager();
    auto previewAction = new QmlPreviewAction();
    designerActionManager.addDesignerAction(new ActionGroup(
                                                QString(),
                                                ComponentCoreConstants::qmlPreviewCategory,
                                                ComponentCoreConstants::priorityQmlPreviewCategory,
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

    auto separator = new SeperatorDesignerAction(ComponentCoreConstants::qmlPreviewCategory, 0);
    designerActionManager.addDesignerAction(separator);

    m_previewToggleAction = previewAction->defaultAction();

    if (s_previewPlugin) {
        auto fpsAction = new FpsAction;
        designerActionManager.addDesignerAction(fpsAction);
        s_previewPlugin->setProperty("fpsHandler", QVariant::fromValue<QmlPreview::QmlPreviewFpsHandler>(FpsLabelAction::fpsHandler));
        auto switchLanguageAction = new SwitchLanguageAction;
        designerActionManager.addDesignerAction(switchLanguageAction);
    }
}

QString QmlPreviewPlugin::pluginName() const
{
    return QLatin1String("QmlPreviewPlugin");
}

void QmlPreviewPlugin::stopAllRunControls()
{
    QTC_ASSERT(s_previewPlugin, return);

    const QVariant variant = s_previewPlugin->property("runningPreviews");
    auto runControls = variant.value<QmlPreview::QmlPreviewRunControlList>();

    for (ProjectExplorer::RunControl *runControl : runControls)
        runControl->initiateStop();

}

void QmlPreviewPlugin::handleRunningPreviews()
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

QString QmlPreviewPlugin::metaInfo() const
{
    return QLatin1String(":/qmlpreviewplugin/qmlpreview.metainfo");
}

void QmlPreviewPlugin::setQmlFile()
{
    if (s_previewPlugin) {
        const Utils::FilePath qmlFileName =
                QmlDesignerPlugin::instance()->currentDesignDocument()->fileName();
        s_previewPlugin->setProperty("previewedFile", qmlFileName.toString());
    }
}

float QmlPreviewPlugin::zoomFactor()
{
    QVariant zoomFactorVariant = 1.0;
    if (s_previewPlugin && !s_previewPlugin->property("zoomFactor").isNull())
        zoomFactorVariant = s_previewPlugin->property("zoomFactor");
    return zoomFactorVariant.toFloat();
}

void QmlPreviewPlugin::setZoomFactor(float zoomFactor)
{
    if (s_previewPlugin)
        s_previewPlugin->setProperty("zoomFactor", zoomFactor);
}

void QmlPreviewPlugin::setLanguageLocale(const QString &locale)
{
    if (auto s_previewPlugin = getPreviewPlugin())
        s_previewPlugin->setProperty("locale", locale);
}

QObject *QmlPreviewPlugin::getPreviewPlugin()
{
    auto pluginIt = std::find_if(ExtensionSystem::PluginManager::plugins().begin(),
                                 ExtensionSystem::PluginManager::plugins().end(),
                                 [](const ExtensionSystem::PluginSpec *p) {
        return p->name() == "QmlPreview";
    });

    if (pluginIt != ExtensionSystem::PluginManager::plugins().constEnd())
        return (*pluginIt)->plugin();

    return nullptr;
}

} // namespace QmlDesigner
