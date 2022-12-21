// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "assetexporterview.h"

#include "qmlitemnode.h"
#include "rewriterview.h"

#include "coreplugin/editormanager/editormanager.h"
#include "coreplugin/editormanager/ieditor.h"
#include "coreplugin/modemanager.h"
#include "coreplugin/coreconstants.h"

#include <QLoggingCategory>

namespace  {
Q_LOGGING_CATEGORY(loggerInfo, "qtc.designer.assetExportPlugin.view", QtInfoMsg)
Q_LOGGING_CATEGORY(loggerWarn, "qtc.designer.assetExportPlugin.view", QtWarningMsg)
//Q_LOGGING_CATEGORY(loggerError, "qtc.designer.assetExportPlugin.view", QtCriticalMsg)

const int RetryIntervalMs = 500;
const int MinRetry = 2;
}

namespace QmlDesigner {

AssetExporterView::AssetExporterView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_timer(this)
{
    m_timer.setInterval(RetryIntervalMs);
    // We periodically check if file is loaded.
    connect(&m_timer, &QTimer::timeout, this, &AssetExporterView::handleTimerTimeout);
}


bool AssetExporterView::loadQmlFile(const Utils::FilePath &path, uint timeoutSecs)
{
    qCDebug(loggerInfo) << "Load file" << path;
    if (loadingState() == LoadState::Busy)
        return false;

    setState(LoadState::Busy);
    m_retryCount = std::max(MinRetry, static_cast<int>((timeoutSecs * 1000) / RetryIntervalMs));
    m_currentEditor = Core::EditorManager::openEditor(path, Utils::Id(),
                                    Core::EditorManager::DoNotMakeVisible);
    Core::ModeManager::activateMode(Core::Constants::MODE_DESIGN);
    Core::ModeManager::setFocusToCurrentMode();
    m_timer.start();
    return true;
}

bool AssetExporterView::saveQmlFile(QString *error) const
{
    if (!m_currentEditor) {
        qCDebug(loggerWarn) << "Saving QML file failed. No editor.";
        return false;
    }
    return m_currentEditor->document()->save(error);
}

void AssetExporterView::modelAttached(Model *model)
{
    if (model->rewriterView() && !model->rewriterView()->errors().isEmpty())
        setState(LoadState::QmlErrorState);

    AbstractView::modelAttached(model);
}

void AssetExporterView::
instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    if (inErrorState() || loadingState() == LoadState::Loaded)
        return; // Already reached error or connected state.

    // We expect correct dimensions are available if the rootnode's
    // information change message is received.
    const auto nodes = informationChangeHash.keys();
    bool hasRootNode = std::any_of(nodes.begin(), nodes.end(), [](const ModelNode &n) {
        return n.isRootNode();
    });
    if (hasRootNode)
        handleMaybeDone();
}

void AssetExporterView::instancesPreviewImageChanged([[maybe_unused]] const QVector<ModelNode> &nodeList)
{
    emit previewChanged();
}

bool AssetExporterView::inErrorState() const
{
    return m_state == LoadState::Exausted || m_state == LoadState::QmlErrorState;
}

bool AssetExporterView::isLoaded() const
{
    return isAttached() && QmlItemNode(rootModelNode()).isValid();
}

void AssetExporterView::setState(AssetExporterView::LoadState state)
{
    if (state != m_state) {
        m_state = state;
        qCDebug(loggerInfo) << "Loading state changed" << m_state;
        if (inErrorState() || m_state == LoadState::Loaded) {
            m_timer.stop();
            // TODO: Send the loaded signal with a delay. The assumption that model attached and a
            // valid root object is enough to declare a QML file is ready is incorrect. A ideal
            // solution would be that the puppet notifies file ready signal.
            if (m_state == LoadState::Loaded)
                QTimer::singleShot(2000, this, &AssetExporterView::loadingFinished);
            else
                emit loadingError(m_state);
        }
    }
}

void AssetExporterView::handleMaybeDone()
{
    if (isLoaded())
        setState(LoadState::Loaded);
}

void AssetExporterView::handleTimerTimeout()
{
    if (!inErrorState() && loadingState() != LoadState::Loaded)
        handleMaybeDone();

    if (--m_retryCount < 0)
        setState(LoadState::Exausted);
}

}

QT_BEGIN_NAMESPACE
QDebug operator<<(QDebug os, const QmlDesigner::AssetExporterView::LoadState &s)
{
    os << static_cast<std::underlying_type<QmlDesigner::AssetExporterView::LoadState>::type>(s);
    return os;
}
QT_END_NAMESPACE
