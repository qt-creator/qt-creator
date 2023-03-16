// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "bakelights.h"

#include "abstractview.h"
#include "bakelightsconnectionmanager.h"
#include "documentmanager.h"
#include "modelnode.h"
#include "nodeabstractproperty.h"
#include "nodeinstanceview.h"
#include "nodemetainfo.h"
#include "plaintexteditmodifier.h"
#include "rewriterview.h"
#include "variantproperty.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <coreplugin/icore.h>

#include <QEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QVariant>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

static QString qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/edit3dQmlSource";
#endif
    return Core::ICore::resourcePath("qmldesigner/edit3dQmlSource").toString();
}

BakeLights::BakeLights(AbstractView *view)
    : QObject(view)
    , m_view(view)
{
    m_view3dId = resolveView3dId(view);

    if (m_view3dId.isEmpty()) {
        // It should never get here, baking controls should be disabled in this case
        qWarning() << __FUNCTION__ << "Active scene is not View3D";
        deleteLater();
        return;
    }

    // Create folders for lightmaps if they do not exist
    PropertyName loadPrefixPropName{"loadPrefix"};
    const QList<ModelNode> bakedLightmapNodes = m_view->allModelNodesOfType(
                m_view->model()->qtQuick3DBakedLightmapMetaInfo());
    Utils::FilePath currentPath = DocumentManager::currentFilePath().absolutePath();
    QSet<Utils::FilePath> pathSet;
    for (const ModelNode &node : bakedLightmapNodes) {
        if (node.hasVariantProperty(loadPrefixPropName)) {
            QString prefix = node.variantProperty(loadPrefixPropName).value().toString();
            Utils::FilePath fp = Utils::FilePath::fromString(prefix);
            if (fp.isRelativePath()) {
                fp = currentPath.pathAppended(prefix);
                if (!fp.exists())
                    pathSet.insert(fp);
            }
        }
    }
    for (const Utils::FilePath &fp : std::as_const(pathSet))
        fp.createDir();

    // Show non-modal progress dialog with cancel button
    QString path = qmlSourcesPath() + "/BakeLightsDialog.qml";

    m_dialog = new QQuickView;
    m_dialog->setTitle(tr("Bake Lights"));
    m_dialog->setResizeMode(QQuickView::SizeRootObjectToView);
    m_dialog->setMinimumSize({150, 100});
    m_dialog->setWidth(800);
    m_dialog->setHeight(400);
    m_dialog->setFlags(Qt::Dialog);
    m_dialog->setModality(Qt::NonModal);
    m_dialog->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");

    m_dialog->rootContext()->setContextProperties({
        {"rootView", QVariant::fromValue(this)},
        {"sceneId", QVariant::fromValue(m_view3dId)}
    });
    m_dialog->setSource(QUrl::fromLocalFile(path));
    m_dialog->installEventFilter(this);
    m_dialog->show();

    QTimer::singleShot(0, this, &BakeLights::bakeLights);
}

BakeLights::~BakeLights()
{
    if (m_connectionManager) {
        m_connectionManager->setProgressCallback({});
        m_connectionManager->setFinishedCallback({});
        m_connectionManager->setCrashCallback({});
    }

    if (m_model) {
        m_model->setNodeInstanceView({});
        m_model->setRewriterView({});
        m_model.reset();
    }

    delete m_dialog;
    delete m_rewriterView;
    delete m_nodeInstanceView;
    delete m_connectionManager;
}

QString BakeLights::resolveView3dId(AbstractView *view)
{
    if (!view || !view->model())
        return {};

    QString view3dId;
    ModelNode activeView3D;
    ModelNode activeScene = view->active3DSceneNode();

    if (activeScene.isValid()) {
        if (activeScene.metaInfo().isQtQuick3DView3D()) {
            activeView3D = activeScene;
        } else {
            ModelNode sceneParent = activeScene.parentProperty().parentModelNode();
            if (sceneParent.metaInfo().isQtQuick3DView3D())
                activeView3D = sceneParent;
        }
        view3dId = activeView3D.id();
    }

    return view3dId;
}

void BakeLights::raiseDialog()
{
    if (m_dialog)
        m_dialog->raise();
}

void BakeLights::bakeLights()
{
    if (!m_view || !m_view->model())
        return;

    // Start baking process
    m_connectionManager = new BakeLightsConnectionManager;
    m_rewriterView = new RewriterView{m_view->externalDependencies(), RewriterView::Amend};
    m_nodeInstanceView = new NodeInstanceView{*m_connectionManager, m_view->externalDependencies()};

    m_model = QmlDesigner::Model::create("QtQuick/Item", 2, 1);
    m_model->setFileUrl(m_view->model()->fileUrl());

    // Take the current unsaved state of the main model and apply it to our copy
    auto textDocument = std::make_unique<QTextDocument>(
                m_view->model()->rewriterView()->textModifier()->textDocument()->toRawText());

    auto modifier = std::make_unique<NotIndentingTextEditModifier>(textDocument.get(),
                                                                   QTextCursor{textDocument.get()});

    m_rewriterView->setTextModifier(modifier.get());
    m_model->setRewriterView(m_rewriterView);

    auto rootModelNodeMetaInfo = m_rewriterView->rootModelNode().metaInfo();
    bool is3DRoot = m_rewriterView->errors().isEmpty()
                    && (rootModelNodeMetaInfo.isQtQuick3DNode()
                        || rootModelNodeMetaInfo.isQtQuick3DMaterial());

    if (!m_rewriterView->errors().isEmpty()
            || (!m_rewriterView->rootModelNode().metaInfo().isGraphicalItem() && !is3DRoot)) {
        emit progress(tr("Invalid root node, baking aborted."));
        emit finished();
        m_dialog->raise();
        return;
    }

    m_nodeInstanceView->setTarget(m_view->nodeInstanceView()->target());

    auto progressCallback = [this](const QString &msg) {
        emit progress(msg);
    };

    auto finishedCallback = [this](const QString &msg) {
        m_dialog->raise();
        emit progress(msg);
        emit finished();

        // Puppet reset is needed to update baking results to current views
        m_view->resetPuppet();
    };

    auto crashCallback = [this]() {
        m_dialog->raise();
        emit progress(tr("Baking process crashed, baking aborted."));
        emit finished();
    };

    m_connectionManager->setProgressCallback(std::move(progressCallback));
    m_connectionManager->setFinishedCallback(std::move(finishedCallback));
    m_connectionManager->setCrashCallback(std::move(crashCallback));

    m_model->setNodeInstanceView(m_nodeInstanceView);

    // InternalIds are not guaranteed to match between normal model and our copy of it, so
    // we identify the View3D by its qml id.
    m_nodeInstanceView->view3DAction(View3DActionType::SetBakeLightsView3D, m_view3dId);
}

void BakeLights::cancel()
{
    if (!m_dialog.isNull() && m_dialog->isVisible())
        m_dialog->close();

    deleteLater();
}

bool BakeLights::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_dialog) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape)
                cancel();
        } else if (event->type() == QEvent::Close) {
            cancel();
        }
    }

    return QObject::eventFilter(obj, event);
}

} // namespace QmlDesigner
