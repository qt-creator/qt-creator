// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerwidget.h"

#include "effectmakercontextobject.h"
#include "effectmakermodel.h"
#include "effectmakernodesmodel.h"
#include "effectmakerview.h"
#include "effectutils.h"
#include "propertyhandler.h"

//#include "qmldesigner/designercore/imagecache/midsizeimagecacheprovider.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "theme.h"

#include <coreplugin/icore.h>

#include <qmldesigner/documentmanager.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmldesigner/qmldesignerplugin.h>
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

namespace EffectMaker {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

EffectMakerWidget::EffectMakerWidget(EffectMakerView *view)
    : m_effectMakerModel{new EffectMakerModel(this)}
    , m_effectMakerNodesModel{new EffectMakerNodesModel(this)}
    , m_effectMakerView(view)
    , m_quickWidget{new StudioQuickWidget(this)}
{
    setWindowTitle(tr("Effect Maker", "Title of effect maker widget"));
    setMinimumWidth(250);

    m_quickWidget->quickWidget()->installEventFilter(this);

    // create the inner widget
    m_quickWidget->quickWidget()->setObjectName(QmlDesigner::Constants::OBJECT_NAME_EFFECT_MAKER);
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

    QmlDesigner::QmlDesignerPlugin::trackWidgetFocusTime(this, QmlDesigner::Constants::EVENT_EFFECTMAKER_TIME);

    m_quickWidget->rootContext()->setContextProperty("g_propertyData", &g_propertyData);

    QString blurPath = "file:" + EffectUtils::nodesSourcesPath() + "/common/";
    g_propertyData.insert(QString("blur_vs_path"), QString(blurPath + "bluritems.vert.qsb"));
    g_propertyData.insert(QString("blur_fs_path"), QString(blurPath + "bluritems.frag.qsb"));

    auto map = m_quickWidget->registerPropertyMap("EffectMakerBackend");
    map->setProperties({{"effectMakerNodesModel", QVariant::fromValue(m_effectMakerNodesModel.data())},
                        {"effectMakerModel", QVariant::fromValue(m_effectMakerModel.data())},
                        {"rootView", QVariant::fromValue(this)}});
    QmlDesigner::QmlDesignerPlugin::trackWidgetFocusTime(
        this, QmlDesigner::Constants::EVENT_NEWEFFECTMAKER_TIME);

    connect(m_effectMakerModel.data(), &EffectMakerModel::nodesChanged, this, [this]() {
        m_effectMakerNodesModel->updateCanBeAdded(m_effectMakerModel->uniformNames());
    });

    connect(m_effectMakerModel.data(), &EffectMakerModel::resourcesSaved,
            this, [this](const QmlDesigner::TypeName &type, const Utils::FilePath &path) {
        if (!m_importScan.timer) {
            m_importScan.timer = new QTimer(this);
            connect(m_importScan.timer, &QTimer::timeout,
                    this, &EffectMakerWidget::handleImportScanTimer);
        }

        if (m_importScan.timer->isActive() && !m_importScan.future.isFinished())
            m_importScan.future.cancel();

        m_importScan.counter = 0;
        m_importScan.type = type;
        m_importScan.path = path;

        m_importScan.timer->start(100);
    });
}


bool EffectMakerWidget::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)
    Q_UNUSED(event)

    // TODO

    return false;
}

void EffectMakerWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    Q_UNUSED(callback)
}

StudioQuickWidget *EffectMakerWidget::quickWidget() const
{
    return m_quickWidget.data();
}

QPointer<EffectMakerModel> EffectMakerWidget::effectMakerModel() const
{
    return m_effectMakerModel;
}

QPointer<EffectMakerNodesModel> EffectMakerWidget::effectMakerNodesModel() const
{
    return m_effectMakerNodesModel;
}

void EffectMakerWidget::addEffectNode(const QString &nodeQenPath)
{
    m_effectMakerModel->addNode(nodeQenPath);
}

void EffectMakerWidget::focusSection(int section)
{
    Q_UNUSED(section)
}

QRect EffectMakerWidget::screenRect() const
{
    if (m_quickWidget && m_quickWidget->screen())
        return m_quickWidget->screen()->availableGeometry();
    return  {};
}

QPoint EffectMakerWidget::globalPos(const QPoint &point) const
{
    if (m_quickWidget)
        return m_quickWidget->mapToGlobal(point);
    return point;
}

QSize EffectMakerWidget::sizeHint() const
{
    return {420, 420};
}

QString EffectMakerWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/effectMakerQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/effectMakerQmlSources").toString();
}

void EffectMakerWidget::initView()
{
    auto ctxObj = new EffectMakerContextObject(m_quickWidget->rootContext());
    m_quickWidget->rootContext()->setContextObject(ctxObj);

    m_backendModelNode.setup(m_effectMakerView->rootModelNode());
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

void EffectMakerWidget::openComposition(const QString &path)
{
    m_compositionPath = path;

    if (effectMakerModel()->hasUnsavedChanges())
        QMetaObject::invokeMethod(quickWidget()->rootObject(), "promptToSaveBeforeOpen");
    else
        doOpenComposition();
}

void EffectMakerWidget::doOpenComposition()
{
    effectMakerModel()->openComposition(m_compositionPath);
}

void EffectMakerWidget::reloadQmlSource()
{
    const QString effectMakerQmlPath = qmlSourcesPath() + "/EffectMaker.qml";
    QTC_ASSERT(QFileInfo::exists(effectMakerQmlPath), return);
    m_quickWidget->setSource(QUrl::fromLocalFile(effectMakerQmlPath));
}

void EffectMakerWidget::handleImportScanTimer()
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
        if (m_effectMakerView->model() && m_effectMakerView->model()->rewriterView()) {
            QmlDesigner::QmlDesignerPlugin::instance()->documentManager().resetPossibleImports();
            m_effectMakerView->model()->rewriterView()->forceAmend();
        }
    } else if (m_importScan.counter == 102) {
        if (m_effectMakerView->model()) {
            // If type is in use, we have to reset puppet to update 2D view
            if (!m_effectMakerView->allModelNodesOfType(
                    m_effectMakerView->model()->metaInfo(m_importScan.type)).isEmpty()) {
                m_effectMakerView->resetPuppet();
            }
        }
    } else if (m_importScan.counter >= 103) {
        // Refresh property view by resetting selection if any selected node is of updated type
        if (m_effectMakerView->model() && m_effectMakerView->hasSelectedModelNodes()) {
            const auto nodes = m_effectMakerView->selectedModelNodes();
            QmlDesigner::MetaInfoType metaType
                = m_effectMakerView->model()->metaInfo(m_importScan.type).type();
            bool match = false;
            for (const QmlDesigner::ModelNode &node : nodes) {
                if (node.metaInfo().type() == metaType) {
                    match = true;
                    break;
                }
            }
            if (match) {
                m_effectMakerView->clearSelectedModelNodes();
                m_effectMakerView->setSelectedModelNodes(nodes);
            }
        }
        m_importScan.timer->stop();
        m_importScan.counter = 0;
    }
}

} // namespace EffectMaker

