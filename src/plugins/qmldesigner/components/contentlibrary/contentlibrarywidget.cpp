// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "contentlibrarywidget.h"

#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialsmodel.h"
#include "contentlibrarytexture.h"
#include "contentlibrarytexturesmodel.h"

#include <coreplugin/icore.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <studioquickwidget.h>
#include <theme.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QJsonDocument>
#include <QMimeData>
#include <QMouseEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWidget>
#include <QShortcut>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

bool ContentLibraryWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (obj == m_quickWidget.data())
            QMetaObject::invokeMethod(m_quickWidget->rootObject(), "closeContextMenu");
    } else if (event->type() == QMouseEvent::MouseMove) {
        DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
        QTC_ASSERT(document, return false);
        Model *model = document->currentModel();
        QTC_ASSERT(model, return false);

        if (m_materialToDrag) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPos() - m_dragStartPoint).manhattanLength() > 20) {
                QByteArray data;
                QMimeData *mimeData = new QMimeData;
                QDataStream stream(&data, QIODevice::WriteOnly);
                stream << m_materialToDrag->type();
                mimeData->setData(Constants::MIME_TYPE_BUNDLE_MATERIAL, data);
                mimeData->removeFormat("text/plain");

                emit bundleMaterialDragStarted(m_materialToDrag);
                model->startDrag(mimeData, m_materialToDrag->icon().toLocalFile());
                m_materialToDrag = nullptr;
            }
        } else if (m_textureToDrag) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPos() - m_dragStartPoint).manhattanLength() > 20
                && m_textureToDrag->isDownloaded()) {
                QMimeData *mimeData = new QMimeData;
                mimeData->setData(Constants::MIME_TYPE_BUNDLE_TEXTURE,
                                  {m_textureToDrag->downloadedTexturePath().toUtf8()});

                // Allows standard file drag-n-drop. As of now needed to drop on Assets view
                mimeData->setUrls({QUrl::fromLocalFile(m_textureToDrag->downloadedTexturePath())});

                emit bundleTextureDragStarted(m_textureToDrag);
                model->startDrag(mimeData, m_textureToDrag->icon().toLocalFile());
                m_textureToDrag = nullptr;
            }
        }
    } else if (event->type() == QMouseEvent::MouseButtonRelease) {
        m_materialToDrag = nullptr;
        m_textureToDrag = nullptr;
        setIsDragging(false);
    }

    return QObject::eventFilter(obj, event);
}

ContentLibraryWidget::ContentLibraryWidget()
    : m_quickWidget(new StudioQuickWidget(this))
    , m_materialsModel(new ContentLibraryMaterialsModel(this))
    , m_texturesModel(new ContentLibraryTexturesModel("Textures", this))
    , m_environmentsModel(new ContentLibraryTexturesModel("Environments", this))
{
    setWindowTitle(tr("Content Library", "Title of content library widget"));
    setMinimumWidth(120);

    m_quickWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_CONTENT_LIBRARY);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->setClearColor(Theme::getColor(Theme::Color::DSpanelBackground));

    QString textureBundlePath = findTextureBundlePath();

    QVariantMap metaData;
    QFile jsonFile(textureBundlePath + "/texture_bundle.json");
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
        metaData = QJsonDocument::fromJson(jsonFile.readAll()).toVariant().toMap();

    m_texturesModel->loadTextureBundle(textureBundlePath, metaData);
    m_environmentsModel->loadTextureBundle(textureBundlePath, metaData);

    Theme::setupTheme(m_quickWidget->engine());
    m_quickWidget->quickWidget()->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_quickWidget.data());

    updateSearch();

    setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F11), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &ContentLibraryWidget::reloadQmlSource);

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_CONTENTLIBRARY_TIME);

    auto map = m_quickWidget->registerPropertyMap("ContentLibraryBackend");

    map->setProperties({{"rootView", QVariant::fromValue(this)},
                        {"materialsModel", QVariant::fromValue(m_materialsModel.data())},
                        {"texturesModel", QVariant::fromValue(m_texturesModel.data())},
                        {"environmentsModel", QVariant::fromValue(m_environmentsModel.data())}});

    reloadQmlSource();
}

QList<QToolButton *> ContentLibraryWidget::createToolBarWidgets()
{
    return {};
}

void ContentLibraryWidget::handleSearchFilterChanged(const QString &filterText)
{
    if (filterText != m_filterText) {
        m_filterText = filterText;
        updateSearch();
    }
}

QString ContentLibraryWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/contentLibraryQmlSource";
#endif
    return Core::ICore::resourcePath("qmldesigner/contentLibraryQmlSource").toString();
}

void ContentLibraryWidget::clearSearchFilter()
{
    QMetaObject::invokeMethod(m_quickWidget->rootObject(), "clearSearchFilter");
}

bool ContentLibraryWidget::hasQuick3DImport() const
{
    return m_hasQuick3DImport;
}

void ContentLibraryWidget::setHasQuick3DImport(bool b)
{
    if (b == m_hasQuick3DImport)
        return;

    const bool oldRequired = m_materialsModel->hasRequiredQuick3DImport();
    m_hasQuick3DImport = b;
    const bool newRequired = m_materialsModel->hasRequiredQuick3DImport();

    if (oldRequired != newRequired)
        emit m_materialsModel->hasRequiredQuick3DImportChanged();

    emit hasQuick3DImportChanged();

    m_materialsModel->updateIsEmpty();
}

bool ContentLibraryWidget::hasMaterialLibrary() const
{
    return m_hasMaterialLibrary;
}

void ContentLibraryWidget::setHasMaterialLibrary(bool b)
{
    if (m_hasMaterialLibrary == b)
        return;

    m_hasMaterialLibrary = b;
    emit hasMaterialLibraryChanged();

    m_materialsModel->updateIsEmpty();
}

void ContentLibraryWidget::reloadQmlSource()
{
    const QString materialBrowserQmlPath = qmlSourcesPath() + "/ContentLibrary.qml";

    QTC_ASSERT(QFileInfo::exists(materialBrowserQmlPath), return);

    m_quickWidget->setSource(QUrl::fromLocalFile(materialBrowserQmlPath));
}

void ContentLibraryWidget::updateSearch()
{
    m_materialsModel->setSearchText(m_filterText);
    m_texturesModel->setSearchText(m_filterText);
    m_environmentsModel->setSearchText(m_filterText);
    m_quickWidget->update();
}

void ContentLibraryWidget::setIsDragging(bool val)
{
    if (m_isDragging != val) {
        m_isDragging = val;
        emit isDraggingChanged();
    }
}

QString ContentLibraryWidget::findTextureBundlePath()
{
    QDir texBundleDir;

    if (!qEnvironmentVariable("TEXTURE_BUNDLE_PATH").isEmpty())
        texBundleDir.setPath(qEnvironmentVariable("TEXTURE_BUNDLE_PATH"));
    else if (Utils::HostOsInfo::isMacHost())
        texBundleDir.setPath(QCoreApplication::applicationDirPath() + "/../Resources/texture_bundle");

    // search for matBundleDir from exec dir and up
    if (texBundleDir.dirName() == ".") {
        texBundleDir.setPath(QCoreApplication::applicationDirPath());
        while (!texBundleDir.cd("texture_bundle") && texBundleDir.cdUp())
            ; // do nothing

        if (texBundleDir.dirName() != "texture_bundle") // bundlePathDir not found
            return {};
    }

    return texBundleDir.path();
}

void ContentLibraryWidget::startDragMaterial(QmlDesigner::ContentLibraryMaterial *mat,
                                             const QPointF &mousePos)
{
    m_materialToDrag = mat;
    m_dragStartPoint = mousePos.toPoint();
    setIsDragging(true);
}

void ContentLibraryWidget::startDragTexture(QmlDesigner::ContentLibraryTexture *tex,
                                            const QPointF &mousePos)
{
    m_textureToDrag = tex;
    m_dragStartPoint = mousePos.toPoint();
    setIsDragging(true);
}

void ContentLibraryWidget::addImage(ContentLibraryTexture *tex)
{
    if (!tex->isDownloaded())
        return;

    emit addTextureRequested(tex->downloadedTexturePath(), AddTextureMode::Image);
}

void ContentLibraryWidget::addTexture(ContentLibraryTexture *tex)
{
    if (!tex->isDownloaded())
        return;

    emit addTextureRequested(tex->downloadedTexturePath(), AddTextureMode::Texture);
}

void ContentLibraryWidget::addLightProbe(ContentLibraryTexture *tex)
{
    if (!tex->isDownloaded())
        return;

    emit addTextureRequested(tex->downloadedTexturePath(), AddTextureMode::LightProbe);
}

void ContentLibraryWidget::updateSceneEnvState()
{
    emit updateSceneEnvStateRequested();
}

QPointer<ContentLibraryMaterialsModel> ContentLibraryWidget::materialsModel() const
{
    return m_materialsModel;
}

QPointer<ContentLibraryTexturesModel> ContentLibraryWidget::texturesModel() const
{
    return m_texturesModel;
}

QPointer<ContentLibraryTexturesModel> ContentLibraryWidget::environmentsModel() const
{
    return m_environmentsModel;
}

bool ContentLibraryWidget::markTextureDownloading()
{
    if (m_anyTextureBeingDownloaded)
        return false;

    m_anyTextureBeingDownloaded = true;
    return true; // let the caller know it can begin download
}

void ContentLibraryWidget::markNoTextureDownloading()
{
    m_anyTextureBeingDownloaded = false; // allow other textures to be downloaded
}

} // namespace QmlDesigner
