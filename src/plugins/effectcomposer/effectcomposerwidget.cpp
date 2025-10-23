// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposerwidget.h"

#include "compositionnode.h"
#include "effectcomposercontextobject.h"
#include "effectcomposermodel.h"
#include "effectcomposernodesmodel.h"
#include "effectcomposertr.h"
#include "effectcomposerview.h"
#include "effectshaderscodeeditor.h"
#include "effectutils.h"
#include "propertyhandler.h"

#include <modelnodeoperations.h>
#include <qmlitemnode.h>

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <qmldesigner/components/componentcore/theme.h>
#include <qmldesigner/components/propertyeditor/assetimageprovider.h>
#include <qmldesigner/designermcumanager.h>
#include <qmldesigner/documentmanager.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmldesigner/qmldesignerplugin.h>
#include <qmldesignerutils/asset.h>
#include <studioquickwidget.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

using namespace Core;

namespace EffectComposer {

constexpr char qmlEffectComposerContextId[] = "QmlDesigner::EffectComposer";
static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toUrlishString();
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
    , m_effectComposerView(view)
    , m_quickWidget{new StudioQuickWidget(this)}
    , m_editor(Utils::makeUniqueObjectLatePtr<EffectShadersCodeEditor>(
          Tr::tr("Shaders Code Editor"), Core::ICore::dialogParent()))
{
    setWindowTitle(Tr::tr("Effect Composer", "Title of effect composer widget"));
    setMinimumWidth(400);

    setupCodeEditor();

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
        Utils::FileUtils::fetchQrc(":/qmldesigner/stylesheet.css")));

    QmlDesigner::QmlDesignerPlugin::trackWidgetFocusTime(this, QmlDesigner::Constants::EVENT_EFFECTCOMPOSER_TIME);

    qmlRegisterSingletonInstance<QQmlPropertyMap>(
        "EffectComposerPropertyData", 1, 0, "GlobalPropertyData", g_propertyData());

    QString blurPath = "file:" + EffectUtils::nodesSourcesPath() + "/common/";
    g_propertyData()->insert("blur_vs_path", QString(blurPath + "bluritems.vert.qsb"));
    g_propertyData()->insert("blur_fs_path", QString(blurPath + "bluritems.frag.qsb"));

    auto map = m_quickWidget->registerPropertyMap("EffectComposerBackend");
    map->setProperties({{"effectComposerNodesModel", QVariant::fromValue(effectComposerNodesModel().data())},
                        {"effectComposerModel", QVariant::fromValue(m_effectComposerModel.data())},
                        {"rootView", QVariant::fromValue(this)}});

    connect(m_effectComposerModel.data(), &EffectComposerModel::resourcesSaved,
            this, [this](const QmlDesigner::TypeName &type) {
        if (!m_effectComposerView->model())
            return;

        QmlDesigner::NodeMetaInfo newTypeInfo = m_effectComposerView->model()->metaInfo(type);
        if (!newTypeInfo.isValid())
            return;

        // If an existing type is updated, we have to reset QML Puppet to update 2D view/properties
        m_effectComposerView->resetPuppet();
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

    connect(
        m_effectComposerModel.data(),
        &EffectComposerModel::modelReset,
        this,
        &EffectComposerWidget::updateCodeEditorIndex);

    connect(
        m_effectComposerModel.data(),
        &EffectComposerModel::rowsMoved,
        this,
        &EffectComposerWidget::updateCodeEditorIndex);

    connect(Core::EditorManager::instance(), &Core::EditorManager::aboutToSave, this, [this] {
        if (m_effectComposerModel->hasUnsavedChanges()) {
            QString compName = m_effectComposerModel->currentComposition();
            if (!compName.isEmpty())
                m_effectComposerModel->saveComposition(compName);
        }
    });

    IContext::attach(this,
                     Context(qmlEffectComposerContextId,
                             QmlDesigner::Constants::qtQuickToolsMenuContextId),
                     [this](const IContext::HelpCallback &callback) { contextHelp(callback); });
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
    return m_effectComposerModel->effectComposerNodesModel();
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

void EffectComposerWidget::removeEffectNodeFromLibrary(const QString &nodeName)
{
    effectComposerNodesModel()->removeEffectNode(nodeName);
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
    return effectComposerNodesModel()->defaultImagesForNode(nodeName).value(uniformName);
}

QString EffectComposerWidget::imagesPath() const
{
    return Core::ICore::resourcePath("qmldesigner/effectComposerNodes/images").toUrlishString();
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

void EffectComposerWidget::updateCanBeAdded()
{
    effectComposerNodesModel()->updateCanBeAdded(m_effectComposerModel->uniformNames(),
                                                 m_effectComposerModel->nodeNames());
}

bool EffectComposerWidget::isMCUProject() const
{
    return QmlDesigner::DesignerMcuManager::instance().isMCUProject();
}

void EffectComposerWidget::openCodeEditor(int idx)
{
    ShaderEditorData *editorData = [&]() -> ShaderEditorData * {
        auto creatorFunction
            = std::bind_front(&EffectShadersCodeEditor::createEditorData, m_editor.get());

        if (idx == MAIN_CODE_EDITOR_INDEX)
            return effectComposerModel()->editorData(creatorFunction);
        else if (auto node = effectComposerModel()->nodeAt(idx))
            return node->editorData(creatorFunction);
        return nullptr;
    }();

    if (!editorData)
        return;

    m_editor->setupShader(editorData);
    m_editor->showWidget();

    updateCodeEditorIndex();
}

void EffectComposerWidget::closeCodeEditor()
{
    m_editor->close();
}

void EffectComposerWidget::openNearestAvailableCodeEditor(int idx)
{
    int nearestIdx = idx;

    if (int rows = m_effectComposerModel->rowCount(); nearestIdx >= rows)
        nearestIdx = rows - 1;

    while (nearestIdx >= 0) {
        CompositionNode *node = m_effectComposerModel->nodeAt(nearestIdx);
        if (!node->isDependency())
            return openCodeEditor(nearestIdx);

        --nearestIdx;
    }

    openCodeEditor(MAIN_CODE_EDITOR_INDEX);
}

QSize EffectComposerWidget::sizeHint() const
{
    return {420, 420};
}

void EffectComposerWidget::setupCodeEditor()
{
    EffectShadersCodeEditor *editor = m_editor.get();
    EffectComposerModel *model = m_effectComposerModel.get();

    editor->setCompositionsModel(model);

    connect(
        editor,
        &EffectShadersCodeEditor::liveUpdateChanged,
        model,
        &EffectComposerModel::setLiveUpdateMode);

    connect(
        editor,
        &EffectShadersCodeEditor::rebakeRequested,
        model,
        &EffectComposerModel::startRebakeTimer);

    connect(
        editor,
        &EffectShadersCodeEditor::openedChanged,
        this,
        &EffectComposerWidget::updateCodeEditorIndex);

    connect(
        model,
        &EffectComposerModel::currentCompositionChanged,
        editor,
        &EffectShadersCodeEditor::close);

    connect(
        editor,
        &EffectShadersCodeEditor::requestToOpenNode,
        this,
        &EffectComposerWidget::openCodeEditor);

    model->setLiveUpdateMode(editor->liveUpdate());
}

QString EffectComposerWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/effectComposerQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/effectComposerQmlSources").toUrlishString();
}

void EffectComposerWidget::initView()
{
    auto ctxObj = new EffectComposerContextObject(m_quickWidget->rootContext());
    m_quickWidget->rootContext()->setContextObject(ctxObj);

    m_backendModelNode.setup(m_effectComposerView->rootModelNode());
    m_quickWidget->rootContext()->setContextProperty("anchorBackend", &m_backendAnchorBinding);
    m_quickWidget->rootContext()->setContextProperty("modelNodeBackend", &m_backendModelNode);
    m_quickWidget->rootContext()->setContextProperty("activeDragSuffix", "");

   m_quickWidget->engine()->addImageProvider("qmldesigner_thumbnails",
                                             new QmlDesigner::AssetImageProvider(
                                                 QmlDesigner::QmlDesignerPlugin::imageCache()));

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

void EffectComposerWidget::updateCodeEditorIndex()
{
    if (m_editor->isOpened()) {
        if (auto editorData = m_editor->currentEditorData())
            m_effectComposerModel->updateCodeEditorIndex(editorData);
        else
            openNearestAvailableCodeEditor(m_effectComposerModel->codeEditorIndex());
    } else {
        m_effectComposerModel->updateCodeEditorIndex(nullptr);
    }
}

} // namespace EffectComposer
