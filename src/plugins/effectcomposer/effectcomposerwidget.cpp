// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposerwidget.h"

#include "effectcomposercontextobject.h"
#include "effectcomposermodel.h"
#include "effectcomposernodesmodel.h"
#include "effectcomposerview.h"
#include "effectutils.h"
#include "propertyhandler.h"

#include <modelnodeoperations.h>
#include <qmlitemnode.h>

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <qmldesigner/components/componentcore/theme.h>
#include <qmldesigner/documentmanager.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmldesigner/qmldesignerplugin.h>
#include <qmldesignerutils/asset.h>
#include <studioquickwidget.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTimer>

namespace EffectComposer {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

static QList<QmlDesigner::ModelNode> modelNodesFromMimeData(const QByteArray &mimeData,
                                                            QmlDesigner::AbstractView *view)
{
    QByteArray encodedModelNodeData = mimeData;
    QDataStream modelNodeStream(&encodedModelNodeData, QIODevice::ReadOnly);

    QList<QmlDesigner::ModelNode> modelNodeList;
    while (!modelNodeStream.atEnd()) {
        qint32 internalId;
        modelNodeStream >> internalId;
        if (view->hasModelNodeForInternalId(internalId))
            modelNodeList.append(view->modelNodeForInternalId(internalId));
    }

    return modelNodeList;
}

EffectComposerWidget::EffectComposerWidget(EffectComposerView *view)
    : m_effectComposerModel{new EffectComposerModel(this)}
    , m_effectComposerNodesModel{new EffectComposerNodesModel(this)}
    , m_effectComposerView(view)
    , m_quickWidget{new StudioQuickWidget(this)}
{
    setWindowTitle(tr("Effect Composer", "Title of effect composer widget"));
    setMinimumWidth(250);

    // create the inner widget
    m_quickWidget->quickWidget()->setObjectName(QmlDesigner::Constants::OBJECT_NAME_EFFECT_COMPOSER);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QmlDesigner::Theme::setupTheme(m_quickWidget->engine());
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->engine()->addImportPath(EffectUtils::nodesSourcesPath() + "/common");
    m_quickWidget->setClearColor(QmlDesigner::Theme::getColor(
        QmlDesigner::Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_quickWidget.data());

    setStyleSheet(QmlDesigner::Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    QmlDesigner::QmlDesignerPlugin::trackWidgetFocusTime(this, QmlDesigner::Constants::EVENT_EFFECTCOMPOSER_TIME);

    m_quickWidget->rootContext()->setContextProperty("g_propertyData", &g_propertyData);

    QString blurPath = "file:" + EffectUtils::nodesSourcesPath() + "/common/";
    g_propertyData.insert(QString("blur_vs_path"), QString(blurPath + "bluritems.vert.qsb"));
    g_propertyData.insert(QString("blur_fs_path"), QString(blurPath + "bluritems.frag.qsb"));

    auto map = m_quickWidget->registerPropertyMap("EffectComposerBackend");
    map->setProperties({{"effectComposerNodesModel", QVariant::fromValue(m_effectComposerNodesModel.data())},
                        {"effectComposerModel", QVariant::fromValue(m_effectComposerModel.data())},
                        {"rootView", QVariant::fromValue(this)}});

    connect(m_effectComposerModel.data(), &EffectComposerModel::nodesChanged, this, [this]() {
        m_effectComposerNodesModel->updateCanBeAdded(m_effectComposerModel->uniformNames());
    });

    connect(m_effectComposerModel.data(), &EffectComposerModel::resourcesSaved,
            this, [this](const QmlDesigner::TypeName &type, const Utils::FilePath &path) {
        if (!m_importScan.timer) {
            m_importScan.timer = new QTimer(this);
            connect(m_importScan.timer, &QTimer::timeout,
                    this, &EffectComposerWidget::handleImportScanTimer);
        }

        if (m_importScan.timer->isActive() && !m_importScan.future.isFinished())
            m_importScan.future.cancel();

        m_importScan.counter = 0;
        m_importScan.type = type;
        m_importScan.path = path;

        m_importScan.timer->start(100);
    });

    connect(m_effectComposerModel.data(), &EffectComposerModel::hasUnsavedChangesChanged,
            this, [this] {
        if (m_effectComposerModel->hasUnsavedChanges() && !m_effectComposerModel->currentComposition().isEmpty()) {
            if (auto doc = QmlDesigner::QmlDesignerPlugin::instance()->documentManager().currentDesignDocument())
                doc->setModified();
        }
    });

    connect(m_effectComposerModel.data(), &EffectComposerModel::modelAboutToBeReset,
            this, [this] {
        QMetaObject::invokeMethod(quickWidget()->rootObject(), "storeExpandStates");
    });

    connect(Core::EditorManager::instance(), &Core::EditorManager::aboutToSave, this, [this] {
        if (m_effectComposerModel->hasUnsavedChanges()) {
            QString compName = m_effectComposerModel->currentComposition();
            if (!compName.isEmpty())
                m_effectComposerModel->saveComposition(compName);
        }
    });
}

void EffectComposerWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    Q_UNUSED(callback)
}

StudioQuickWidget *EffectComposerWidget::quickWidget() const
{
    return m_quickWidget.data();
}

QPointer<EffectComposerModel> EffectComposerWidget::effectComposerModel() const
{
    return m_effectComposerModel;
}

QPointer<EffectComposerNodesModel> EffectComposerWidget::effectComposerNodesModel() const
{
    return m_effectComposerNodesModel;
}

void EffectComposerWidget::addEffectNode(const QString &nodeQenPath)
{
    m_effectComposerModel->addNode(nodeQenPath);

    if (!nodeQenPath.isEmpty()) {
        using namespace QmlDesigner;
        QString id = nodeQenPath.split('/').last().chopped(4).prepend('_');
        QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_EFFECTCOMPOSER_NODE + id);
    }
}

void EffectComposerWidget::focusSection(int section)
{
    Q_UNUSED(section)
}

QRect EffectComposerWidget::screenRect() const
{
    if (m_quickWidget && m_quickWidget->screen())
        return m_quickWidget->screen()->availableGeometry();
    return  {};
}

QPoint EffectComposerWidget::globalPos(const QPoint &point) const
{
    if (m_quickWidget)
        return m_quickWidget->mapToGlobal(point);
    return point;
}

QString EffectComposerWidget::uniformDefaultImage(const QString &nodeName, const QString &uniformName) const
{
    return m_effectComposerNodesModel->defaultImagesForNode(nodeName).value(uniformName);
}

QString EffectComposerWidget::imagesPath() const
{
    return Core::ICore::resourcePath("qmldesigner/effectComposerNodes/images").toString();
}

bool EffectComposerWidget::isEffectAsset(const QUrl &url) const
{
    return QmlDesigner::Asset(url.toLocalFile()).isEffect();
}

void EffectComposerWidget::dropAsset(const QUrl &url)
{
    if (isEffectAsset(url))
        openComposition(url.toLocalFile());
}

bool EffectComposerWidget::isEffectNode(const QByteArray &mimeData) const
{
    QList<QmlDesigner::ModelNode> nodes = modelNodesFromMimeData(mimeData, m_effectComposerView);
    if (!nodes.isEmpty())
        return QmlDesigner::QmlItemNode(nodes.last()).isEffectItem();
    return false;
}

void EffectComposerWidget::dropNode(const QByteArray &mimeData)
{
    QList<QmlDesigner::ModelNode> nodes = modelNodesFromMimeData(mimeData, m_effectComposerView);
    if (!nodes.isEmpty() && QmlDesigner::QmlItemNode(nodes.last()).isEffectItem()) {
        Utils::FilePath path = QmlDesigner::ModelNodeOperations::findEffectFile(nodes.last());
        openComposition(path.toFSPathString());
    }
}

QSize EffectComposerWidget::sizeHint() const
{
    return {420, 420};
}

QString EffectComposerWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/effectComposerQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/effectComposerQmlSources").toString();
}

void EffectComposerWidget::initView()
{
    auto ctxObj = new EffectComposerContextObject(m_quickWidget->rootContext());
    m_quickWidget->rootContext()->setContextObject(ctxObj);

    m_backendModelNode.setup(m_effectComposerView->rootModelNode());
    m_quickWidget->rootContext()->setContextProperty("anchorBackend", &m_backendAnchorBinding);
    m_quickWidget->rootContext()->setContextProperty("modelNodeBackend", &m_backendModelNode);
    m_quickWidget->rootContext()->setContextProperty("activeDragSuffix", "");

    //TODO: Fix crash on macos
//    m_quickWidget->engine()->addImageProvider("qmldesigner_thumbnails",
//                                              new QmlDesigner::AssetImageProvider(
//                                                  QmlDesigner::QmlDesignerPlugin::imageCache()));

    // init the first load of the QML UI elements
    reloadQmlSource();
}

void EffectComposerWidget::openComposition(const QString &path)
{
    m_compositionPath = path;

    if (effectComposerModel()->hasUnsavedChanges())
        QMetaObject::invokeMethod(quickWidget()->rootObject(), "promptToSaveBeforeOpen");
    else
        doOpenComposition();
}

void EffectComposerWidget::doOpenComposition()
{
    effectComposerModel()->openComposition(m_compositionPath);
}

void EffectComposerWidget::reloadQmlSource()
{
    const QString effectComposerQmlPath = qmlSourcesPath() + "/EffectComposer.qml";
    QTC_ASSERT(QFileInfo::exists(effectComposerQmlPath), return);
    m_quickWidget->setSource(QUrl::fromLocalFile(effectComposerQmlPath));
}

void EffectComposerWidget::handleImportScanTimer()
{
    ++m_importScan.counter;

    if (m_importScan.counter == 1) {
        // Rescan the effect import to update code model
        auto modelManager = QmlJS::ModelManagerInterface::instance();
        if (modelManager) {
            QmlJS::PathsAndLanguages pathToScan;
            pathToScan.maybeInsert(m_importScan.path);
            m_importScan.future = ::Utils::asyncRun(&QmlJS::ModelManagerInterface::importScan,
                                                    modelManager->workingCopy(),
                                                    pathToScan, modelManager, true, true, true);
        }
    } else if (m_importScan.counter < 100) {
        // We have to wait a while to ensure qmljs detects new files and updates its
        // internal model. Then we force amend on rewriter to trigger qmljs snapshot update.
        if (m_importScan.future.isCanceled() || m_importScan.future.isFinished())
            m_importScan.counter = 100; // skip the timeout step
    } else if (m_importScan.counter == 100) {
        // Scanning is taking too long, abort
        m_importScan.future.cancel();
        m_importScan.timer->stop();
        m_importScan.counter = 0;
    } else if (m_importScan.counter == 101) {
        if (m_effectComposerView->model() && m_effectComposerView->model()->rewriterView()) {
            QmlDesigner::QmlDesignerPlugin::instance()->documentManager().resetPossibleImports();
            m_effectComposerView->model()->rewriterView()->forceAmend();
        }
    } else if (m_importScan.counter == 102) {
        if (m_effectComposerView->model()) {
            // If type is in use, we have to reset puppet to update 2D view
            if (!m_effectComposerView->allModelNodesOfType(
                                         m_effectComposerView->model()->metaInfo(m_importScan.type)).isEmpty()) {
                m_effectComposerView->resetPuppet();
            }
        }
    } else if (m_importScan.counter >= 103) {
        // Refresh property view by resetting selection if any selected node is of updated type
        if (m_effectComposerView->model() && m_effectComposerView->hasSelectedModelNodes()) {
            const auto nodes = m_effectComposerView->selectedModelNodes();
            QmlDesigner::MetaInfoType metaType
                = m_effectComposerView->model()->metaInfo(m_importScan.type).type();
            bool match = false;
            for (const QmlDesigner::ModelNode &node : nodes) {
                if (node.metaInfo().type() == metaType) {
                    match = true;
                    break;
                }
            }
            if (match) {
                m_effectComposerView->clearSelectedModelNodes();
                m_effectComposerView->setSelectedModelNodes(nodes);
            }
        }
        m_importScan.timer->stop();
        m_importScan.counter = 0;
    }
}

} // namespace EffectComposer

