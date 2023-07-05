// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "bakelights.h"

#include <abstractview.h>
#include <auxiliarydataproperties.h>
#include <bakelightsconnectionmanager.h>
#include <bakelightsdatamodel.h>
#include <bindingproperty.h>
#include <documentmanager.h>
#include <model/modelutils.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>
#include <nodeinstanceview.h>
#include <nodemetainfo.h>
#include <plaintexteditmodifier.h>
#include <rewriterview.h>
#include <variantproperty.h>

#include <coreplugin/icore.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QSaveFile>
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

    showSetupDialog();
}

BakeLights::~BakeLights()
{
    cleanup();
}

ModelNode BakeLights::resolveView3dNode(AbstractView *view)
{
    if (!view || !view->model())
        return {};

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
        return activeView3D;
    }

    return {};
}

QString BakeLights::resolveView3dId(AbstractView *view)
{
    ModelNode activeView3D = resolveView3dNode(view);

    if (activeView3D.isValid())
        return activeView3D.id();

    return {};
}

void BakeLights::raiseDialog()
{
    if (m_progressDialog)
        m_progressDialog->raise();
}

bool BakeLights::manualMode() const
{
    return m_manualMode;
}

void BakeLights::setManualMode(bool enabled)
{
    if (m_manualMode != enabled) {
        m_manualMode = enabled;
        emit manualModeChanged();
    }
}

void BakeLights::bakeLights()
{
    if (!m_view || !m_view->model())
        return;

    m_setupDialog->hide();
    showProgressDialog();

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
        m_progressDialog->raise();
        return;
    }

    m_nodeInstanceView->setTarget(m_view->nodeInstanceView()->target());

    auto progressCallback = [this](const QString &msg) {
        emit progress(msg);
    };

    auto finishedCallback = [this](const QString &msg) {
        m_progressDialog->raise();
        emit progress(msg);
        emit finished();

        // Puppet reset is needed to update baking results to current views
        m_view->resetPuppet();
    };

    auto crashCallback = [this]() {
        m_progressDialog->raise();
        emit progress(tr("Baking process crashed, baking aborted."));
        emit finished();
    };

    m_connectionManager->setProgressCallback(std::move(progressCallback));
    m_connectionManager->setFinishedCallback(std::move(finishedCallback));
    m_nodeInstanceView->setCrashCallback(std::move(crashCallback));

    m_model->setNodeInstanceView(m_nodeInstanceView);

    // InternalIds are not guaranteed to match between normal model and our copy of it, so
    // we identify the View3D by its qml id.
    m_nodeInstanceView->view3DAction(View3DActionType::SetBakeLightsView3D, m_view3dId);
}

void BakeLights::apply()
{
    // Uninitialized QVariant stored instead of false to remove the aux property in that case
    m_dataModel->view3dNode().setAuxiliaryData(bakeLightsManualProperty,
                                               m_manualMode ? QVariant{true} : QVariant{});

    if (!m_manualMode)
        m_dataModel->apply();

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
}

void BakeLights::rebake()
{
    QTimer::singleShot(0, this, [this]() {
        cleanup();
        showSetupDialog();
    });
}

void BakeLights::exposeModelsAndLights(const QString &nodeId)
{
    ModelNode compNode = m_view->modelNodeForId(nodeId);
    if (!compNode.isValid() || !compNode.isComponent()) {
        return;
    }

    auto componentFilePath = ModelUtils::componentFilePath(compNode);
    if (componentFilePath.isEmpty()) {
        return;
    }

    RewriterView rewriter{m_view->externalDependencies(), RewriterView::Amend};
    ModelPointer compModel = QmlDesigner::Model::create("QtQuick/Item", 2, 1);
    const Utils::FilePath compFilePath = Utils::FilePath::fromString(componentFilePath);
    QByteArray src = compFilePath.fileContents().value();

    compModel->setFileUrl(QUrl::fromLocalFile(componentFilePath));

    auto textDocument = std::make_unique<QTextDocument>(QString::fromUtf8(src));
    auto modifier = std::make_unique<IndentingTextEditModifier>(
        textDocument.get(), QTextCursor{textDocument.get()});

    rewriter.setTextModifier(modifier.get());
    compModel->setRewriterView(&rewriter);

    if (!rewriter.rootModelNode().isValid() || !rewriter.errors().isEmpty())
        return;

    QString originalText = modifier->text();
    QStringList idList;

    rewriter.executeInTransaction(__FUNCTION__, [&]() {
        QList<ModelNode> nodes = rewriter.rootModelNode().allSubModelNodes();
        for (ModelNode &node : nodes) {
            if (node.metaInfo().isQtQuick3DModel() || node.metaInfo().isQtQuick3DLight()) {
                QString idStr = node.id();
                if (idStr.isEmpty()) {
                    const QString type = node.metaInfo().isQtQuick3DModel() ? "model" : "light";
                    idStr = compModel->generateNewId(type);
                    node.setIdWithoutRefactoring(idStr);
                }
                idList.append(idStr);
            }
        }
    });

    rewriter.executeInTransaction(__FUNCTION__, [&]() {
        for (const QString &id : std::as_const(idList)) {
            ModelNode node = rewriter.modelNodeForId(id);
            if (!node.isValid())
                continue;
            rewriter.rootModelNode().bindingProperty(id.toUtf8())
                .setDynamicTypeNameAndExpression("alias", id);
        }
    });

    rewriter.forceAmend();

    QString newText = modifier->text();
    if (newText != originalText) {
        QSaveFile saveFile(componentFilePath);
        if (saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            saveFile.write(newText.toUtf8());
            saveFile.commit();
        } else {
            qWarning() << __FUNCTION__ << "Failed to save changes to:" << componentFilePath;
        }
    }

    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    QmlJS::Document::Ptr doc = rewriter.document()->ptr();
    modelManager->updateDocument(doc);

    m_view->model()->rewriterView()->forceAmend();

    compModel->setRewriterView({});

    // Rebake to relaunch setup dialog with updated properties
    rebake();
}

void BakeLights::showSetupDialog()
{
    if (!m_dataModel)
        m_dataModel = new BakeLightsDataModel(m_view);

    if (!m_dataModel->reset()) {
        m_manualMode = true;
    } else {
        auto data = m_dataModel->view3dNode().auxiliaryData(bakeLightsManualProperty);
        if (data)
            m_manualMode = data->toBool();
    }

    if (!m_setupDialog) {
        // Show non-modal progress dialog with cancel button
        QString path = qmlSourcesPath() + "/BakeLightsSetupDialog.qml";

        m_setupDialog = new QQuickView;
        m_setupDialog->setTitle(tr("Lights Baking Setup"));
        m_setupDialog->setResizeMode(QQuickView::SizeRootObjectToView);
        m_setupDialog->setMinimumSize({550, 200});
        m_setupDialog->setWidth(550);
        m_setupDialog->setHeight(400);
        m_setupDialog->setFlags(Qt::Dialog);
        m_setupDialog->setModality(Qt::ApplicationModal);
        m_setupDialog->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");

        m_setupDialog->rootContext()->setContextProperties({
            {"rootView", QVariant::fromValue(this)},
            {"sceneId", QVariant::fromValue(m_view3dId)},
            {"bakeModel", QVariant::fromValue(m_dataModel.data())}
        });
        m_setupDialog->setSource(QUrl::fromLocalFile(path));
        m_setupDialog->installEventFilter(this);
    }
    m_setupDialog->show();
}

void BakeLights::showProgressDialog()
{
    if (!m_progressDialog) {
        // Show non-modal progress dialog with cancel button
        QString path = qmlSourcesPath() + "/BakeLightsProgressDialog.qml";

        m_progressDialog = new QQuickView;
        m_progressDialog->setTitle(tr("Lights Baking Progress"));
        m_progressDialog->setResizeMode(QQuickView::SizeRootObjectToView);
        m_progressDialog->setMinimumSize({150, 100});
        m_progressDialog->setWidth(800);
        m_progressDialog->setHeight(400);
        m_progressDialog->setFlags(Qt::Dialog);
        m_progressDialog->setModality(Qt::NonModal);
        m_progressDialog->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");

        m_progressDialog->rootContext()->setContextProperties({
            {"rootView", QVariant::fromValue(this)},
            {"sceneId", QVariant::fromValue(m_view3dId)}
        });
        m_progressDialog->setSource(QUrl::fromLocalFile(path));
        m_progressDialog->installEventFilter(this);
    }
    m_progressDialog->show();
}

void BakeLights::cleanup()
{
    if (m_connectionManager) {
        m_connectionManager->setProgressCallback({});
        m_connectionManager->setFinishedCallback({});
        m_nodeInstanceView->setCrashCallback({});
    }

    if (m_model) {
        m_model->setNodeInstanceView({});
        m_model->setRewriterView({});
        m_model.reset();
    }

    delete m_setupDialog;
    delete m_progressDialog;
    delete m_rewriterView;
    delete m_nodeInstanceView;
    delete m_connectionManager;
    delete m_dataModel;

    m_manualMode = false;
}

void BakeLights::cancel()
{
    if (!m_setupDialog.isNull() && m_setupDialog->isVisible())
        m_setupDialog->close();
    if (!m_progressDialog.isNull() && m_progressDialog->isVisible())
        m_progressDialog->close();

    deleteLater();
}

bool BakeLights::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_progressDialog || obj == m_setupDialog) {
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
