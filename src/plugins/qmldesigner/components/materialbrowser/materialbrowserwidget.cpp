// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialbrowserwidget.h"

#include "materialbrowsermodel.h"
#include "materialbrowsertexturesmodel.h"
#include "materialbrowserview.h"

#include <designeractionmanager.h>
#include <designermcumanager.h>
#include <documentmanager.h>
#include <propertyeditorimageprovider.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <variantproperty.h>

#include <theme.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hdrimage.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickWidget>
#include <QShortcut>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

class PreviewImageProvider : public QQuickImageProvider
{
    QHash<qint32, QPixmap> m_pixmaps;

public:
    PreviewImageProvider()
        : QQuickImageProvider(Pixmap) {}

    void setPixmap(const ModelNode &node, const QPixmap &pixmap)
    {
        m_pixmaps.insert(node.internalId(), pixmap);
    }

    void clearPixmapCache()
    {
        m_pixmaps.clear();
    }

    QPixmap requestPixmap(const QString &id,
                          QSize *size,
                          [[maybe_unused]] const QSize &requestedSize) override
    {
        static QPixmap defaultPreview = QPixmap::fromImage(QImage(":/materialeditor/images/defaultmaterialpreview.png"));

        QPixmap pixmap{150, 150};

        qint32 internalId = id.toInt();
        if (m_pixmaps.contains(internalId))
            pixmap = m_pixmaps.value(internalId);
        else
            pixmap = defaultPreview;

        if (size)
            *size = pixmap.size();

        return pixmap;
    }
};

bool MaterialBrowserWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (obj == m_quickWidget.data())
            QMetaObject::invokeMethod(m_quickWidget->rootObject(), "closeContextMenu");
    } else if (event->type() == QMouseEvent::MouseMove) {
        DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
        QTC_ASSERT(document, return false);
        Model *model = document->currentModel();
        QTC_ASSERT(model, return false);

        if (m_materialToDrag.isValid() || m_textureToDrag.isValid()) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPos() - m_dragStartPoint).manhattanLength() > 20) {
                bool isMaterial = m_materialToDrag.isValid();
                QMimeData *mimeData = new QMimeData;
                QByteArray internalId;

                if (isMaterial) {
                    internalId.setNum(m_materialToDrag.internalId());
                    mimeData->setData(Constants::MIME_TYPE_MATERIAL, internalId);
                    model->startDrag(mimeData, m_previewImageProvider->requestPixmap(
                                     QString::number(m_materialToDrag.internalId()), nullptr, {128, 128}));
                } else {
                    internalId.setNum(m_textureToDrag.internalId());
                    mimeData->setData(Constants::MIME_TYPE_TEXTURE, internalId);
                    QString iconPath = QLatin1String("%1/%2")
                                    .arg(DocumentManager::currentResourcePath().path(),
                                         m_textureToDrag.variantProperty("source").value().toString());

                    QPixmap pixmap;
                    const QString suffix = iconPath.split('.').last().toLower();
                    if (suffix == "hdr")
                        pixmap = HdrImage{iconPath}.toPixmap();
                    else
                        pixmap = Utils::StyleHelper::dpiSpecificImageFile(iconPath);
                    if (pixmap.isNull())
                        pixmap = Utils::StyleHelper::dpiSpecificImageFile(":/textureeditor/images/texture_default.png");
                    model->startDrag(mimeData, pixmap.scaled({128, 128}));
                }
                m_materialToDrag = {};
                m_textureToDrag = {};
            }
        }
    } else if (event->type() == QMouseEvent::MouseButtonRelease) {
        m_materialToDrag = {};
        m_textureToDrag = {};
    }

    return QObject::eventFilter(obj, event);
}

MaterialBrowserWidget::MaterialBrowserWidget(AsynchronousImageCache &imageCache,
                                             MaterialBrowserView *view)
    : m_materialBrowserView(view)
    , m_materialBrowserModel(new MaterialBrowserModel(this))
    , m_materialBrowserTexturesModel(new MaterialBrowserTexturesModel(this))
    , m_quickWidget(new QQuickWidget(this))
    , m_previewImageProvider(new PreviewImageProvider())
{
    QImage defaultImage;
    defaultImage.load(Utils::StyleHelper::dpiSpecificImageFile(":/textureeditor/images/texture_default.png"));
    m_textureImageProvider = new PropertyEditorImageProvider(imageCache, defaultImage);

    setWindowTitle(tr("Material Browser", "Title of material browser widget"));
    setMinimumWidth(120);

    Core::Context context(Constants::C_QMLMATERIALBROWSER);
    m_context = new Core::IContext(this);
    m_context->setContext(context);
    m_context->setWidget(this);

    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->setClearColor(Theme::getColor(Theme::Color::DSpanelBackground));

    m_quickWidget->rootContext()->setContextProperties({
        {"rootView", QVariant::fromValue(this)},
        {"materialBrowserModel", QVariant::fromValue(m_materialBrowserModel.data())},
        {"materialBrowserTexturesModel", QVariant::fromValue(m_materialBrowserTexturesModel.data())},
    });

    m_quickWidget->engine()->addImageProvider("materialBrowser", m_previewImageProvider);
    m_quickWidget->engine()->addImageProvider("materialBrowserTex", m_textureImageProvider);

    Theme::setupTheme(m_quickWidget->engine());
    m_quickWidget->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_quickWidget.data());

    updateSearch();

    setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F8), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &MaterialBrowserWidget::reloadQmlSource);

    connect(m_materialBrowserModel, &MaterialBrowserModel::isEmptyChanged, this, [&] {
        if (m_materialBrowserModel->isEmpty())
            focusMaterialSection(false);
    });

    connect(m_materialBrowserTexturesModel, &MaterialBrowserTexturesModel::isEmptyChanged, this, [&] {
        if (m_materialBrowserTexturesModel->isEmpty())
            focusMaterialSection(true);
    });

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_MATERIALBROWSER_TIME);

    reloadQmlSource();
}

void MaterialBrowserWidget::updateMaterialPreview(const ModelNode &node, const QPixmap &pixmap)
{
    m_previewImageProvider->setPixmap(node, pixmap);
    int idx = m_materialBrowserModel->materialIndex(node);
    if (idx != -1)
        QMetaObject::invokeMethod(m_quickWidget->rootObject(), "refreshPreview", Q_ARG(QVariant, idx));
}

void MaterialBrowserWidget::deleteSelectedItem()
{
    if (m_materialSectionFocused)
        m_materialBrowserModel->deleteSelectedMaterial();
    else
        m_materialBrowserTexturesModel->deleteSelectedTexture();
}

QList<QToolButton *> MaterialBrowserWidget::createToolBarWidgets()
{
    return {};
}

void MaterialBrowserWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_materialBrowserView)
        QmlDesignerPlugin::contextHelp(callback, m_materialBrowserView->contextHelpId());
    else
        callback({});
}

void MaterialBrowserWidget::handleSearchFilterChanged(const QString &filterText)
{
    if (filterText != m_filterText) {
        m_filterText = filterText;
        updateSearch();
    }
}

void MaterialBrowserWidget::startDragMaterial(int index, const QPointF &mousePos)
{
    m_materialToDrag = m_materialBrowserModel->materialAt(index);
    m_dragStartPoint = mousePos.toPoint();
}

void MaterialBrowserWidget::startDragTexture(int index, const QPointF &mousePos)
{
    m_textureToDrag = m_materialBrowserTexturesModel->textureAt(index);
    m_dragStartPoint = mousePos.toPoint();
}

void MaterialBrowserWidget::acceptBundleMaterialDrop()
{
    m_materialBrowserView->emitCustomNotification("drop_bundle_material", {}, {}); // To ContentLibraryView
}

void MaterialBrowserWidget::acceptBundleTextureDrop()
{
    m_materialBrowserView->emitCustomNotification("drop_bundle_texture", {}, {}); // To ContentLibraryView
}

void MaterialBrowserWidget::acceptTextureDropOnMaterial(int matIndex, const QString &texId)
{
    ModelNode mat = m_materialBrowserModel->materialAt(matIndex);
    ModelNode tex = m_materialBrowserView->modelNodeForInternalId(texId.toInt());

    if (mat.isValid() && tex.isValid()) {
        m_materialBrowserModel->selectMaterial(matIndex);
        m_materialBrowserView->applyTextureToMaterial({mat}, tex);
    }
}

void MaterialBrowserWidget::focusMaterialSection(bool focusMatSec)
{
    if (focusMatSec != m_materialSectionFocused) {
        m_materialSectionFocused = focusMatSec;
        emit materialSectionFocusedChanged();
    }
}

QString MaterialBrowserWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/materialBrowserQmlSource";
#endif
    return Core::ICore::resourcePath("qmldesigner/materialBrowserQmlSource").toString();
}

void MaterialBrowserWidget::clearSearchFilter()
{
    QMetaObject::invokeMethod(m_quickWidget->rootObject(), "clearSearchFilter");
}

void MaterialBrowserWidget::reloadQmlSource()
{
    const QString materialBrowserQmlPath = qmlSourcesPath() + "/MaterialBrowser.qml";

    QTC_ASSERT(QFileInfo::exists(materialBrowserQmlPath), return);

    m_quickWidget->engine()->clearComponentCache();
    m_quickWidget->setSource(QUrl::fromLocalFile(materialBrowserQmlPath));
}

void MaterialBrowserWidget::updateSearch()
{
    m_materialBrowserModel->setSearchText(m_filterText);
    m_materialBrowserTexturesModel->setSearchText(m_filterText);
    m_quickWidget->update();
}

QQuickWidget *MaterialBrowserWidget::quickWidget() const
{
    return m_quickWidget.data();
}

void MaterialBrowserWidget::clearPreviewCache()
{
    m_previewImageProvider->clearPixmapCache();
}

QPointer<MaterialBrowserModel> MaterialBrowserWidget::materialBrowserModel() const
{
    return m_materialBrowserModel;
}

QPointer<MaterialBrowserTexturesModel> MaterialBrowserWidget::materialBrowserTexturesModel() const
{
    return m_materialBrowserTexturesModel;
}

} // namespace QmlDesigner
