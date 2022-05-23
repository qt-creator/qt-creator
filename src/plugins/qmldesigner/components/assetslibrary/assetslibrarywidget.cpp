/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "assetslibrarywidget.h"

#include "assetslibrarymodel.h"
#include "assetslibraryiconprovider.h"

#include <theme.h>

#include <designeractionmanager.h>
#include "modelnodeoperations.h"
#include <model.h>
#include <navigatorwidget.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/filesystemwatcher.h>
#include <utils/stylehelper.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QApplication>
#include <QDrag>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QImageReader>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QShortcut>
#include <QTimer>
#include <QToolButton>
#include <QQmlContext>
#include <QQuickItem>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

bool AssetsLibraryWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (obj == m_assetsWidget.data())
            QMetaObject::invokeMethod(m_assetsWidget->rootObject(), "handleViewFocusOut");
    } else if (event->type() == QMouseEvent::MouseMove) {
        if (!m_assetsToDrag.isEmpty()) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPos() - m_dragStartPoint).manhattanLength() > 10) {
                auto drag = new QDrag(this);
                drag->setPixmap(m_assetsIconProvider->requestPixmap(m_assetsToDrag[0], nullptr, {128, 128}));
                QMimeData *mimeData = new QMimeData;
                mimeData->setData(Constants::MIME_TYPE_ASSETS, m_assetsToDrag.join(',').toUtf8());
                drag->setMimeData(mimeData);
                drag->exec();
                drag->deleteLater();

                m_assetsToDrag.clear();
            }
        }
    } else if (event->type() == QMouseEvent::MouseButtonRelease) {
        m_assetsToDrag.clear();
        QWidget *view = QmlDesignerPlugin::instance()->viewManager().widget("Navigator");
        if (view) {
            NavigatorWidget *navView = qobject_cast<NavigatorWidget *>(view);
            if (navView) {
                navView->setDragType("");
                navView->update();
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

AssetsLibraryWidget::AssetsLibraryWidget(AsynchronousImageCache &asynchronousFontImageCache,
                                         SynchronousImageCache &synchronousFontImageCache)
    : m_itemIconSize(24, 24)
    , m_fontImageCache(synchronousFontImageCache)
    , m_assetsIconProvider(new AssetsLibraryIconProvider(synchronousFontImageCache))
    , m_fileSystemWatcher(new Utils::FileSystemWatcher(this))
    , m_assetsModel(new AssetsLibraryModel(m_fileSystemWatcher, this))
    , m_assetsWidget(new QQuickWidget(this))
{
    m_assetCompressionTimer.setInterval(200);
    m_assetCompressionTimer.setSingleShot(true);

    setWindowTitle(tr("Assets Library", "Title of assets library widget"));
    setMinimumWidth(250);

    m_assetsWidget->installEventFilter(this);

    m_fontPreviewTooltipBackend = std::make_unique<PreviewTooltipBackend>(asynchronousFontImageCache);
    // We want font images to have custom size, so don't scale them in the tooltip
    m_fontPreviewTooltipBackend->setScaleImage(false);
    // Note: Though the text specified here appears in UI, it shouldn't be translated, as it's
    // a commonly used sentence to preview the font glyphs in latin fonts.
    // For fonts that do not have latin glyphs, the font family name will have to suffice for preview.
    m_fontPreviewTooltipBackend->setAuxiliaryData(
        ImageCache::FontCollectorSizeAuxiliaryData{QSize{300, 150},
                                                   Theme::getColor(Theme::DStextColor).name(),
                                                   QStringLiteral("The quick brown fox jumps\n"
                                                                  "over the lazy dog\n"
                                                                  "1234567890")});
    // create assets widget
    m_assetsWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    Theme::setupTheme(m_assetsWidget->engine());
    m_assetsWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_assetsWidget->setClearColor(Theme::getColor(Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));
    m_assetsWidget->engine()->addImageProvider("qmldesigner_assets", m_assetsIconProvider);
    m_assetsWidget->rootContext()->setContextProperties(QVector<QQmlContext::PropertyPair>{
        {{"assetsModel"}, QVariant::fromValue(m_assetsModel.data())},
        {{"rootView"}, QVariant::fromValue(this)},
        {{"tooltipBackend"}, QVariant::fromValue(m_fontPreviewTooltipBackend.get())}
    });

    // If project directory contents change, or one of the asset files is modified, we must
    // reconstruct the model to update the icons
    connect(m_fileSystemWatcher, &Utils::FileSystemWatcher::directoryChanged, [this](const QString & changedDirPath) {
        Q_UNUSED(changedDirPath)
        m_assetCompressionTimer.start();
    });

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_assetsWidget.data());

    updateSearch();

    setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F6), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &AssetsLibraryWidget::reloadQmlSource);

    connect(&m_assetCompressionTimer, &QTimer::timeout, this, [this]() {
        // TODO: find a clever way to only refresh the changed directory part of the model

        // Don't bother with asset updates after model has detached, project is probably closing
        if (!m_model.isNull()) {
            if (QApplication::activeModalWidget()) {
                // Retry later, as updating file system watchers can crash when there is an active
                // modal widget
                m_assetCompressionTimer.start();
            } else {
                m_assetsModel->refresh();
                // reload assets qml so that an overridden file's image shows the new image
                QTimer::singleShot(100, this, &AssetsLibraryWidget::reloadQmlSource);
            }
        }
    });

     QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_ASSETSLIBRARY_TIME);

    // init the first load of the QML UI elements
    reloadQmlSource();
}

AssetsLibraryWidget::~AssetsLibraryWidget() = default;

QList<QToolButton *> AssetsLibraryWidget::createToolBarWidgets()
{
    return {};
}

void AssetsLibraryWidget::handleSearchfilterChanged(const QString &filterText)
{
    if (filterText == m_filterText || (m_assetsModel->isEmpty() && filterText.contains(m_filterText)))
            return;

    m_filterText = filterText;
    updateSearch();
}

void AssetsLibraryWidget::handleAddAsset()
{
    addResources({});
}

void AssetsLibraryWidget::handleExtFilesDrop(const QList<QUrl> &simpleFilePaths,
                                             const QList<QUrl> &complexFilePaths,
                                             const QString &targetDirPath)
{
    auto toLocalFile = [](const QUrl &url) { return url.toLocalFile(); };

    QStringList simpleFilePathStrings = Utils::transform<QStringList>(simpleFilePaths, toLocalFile);
    QStringList complexFilePathStrings = Utils::transform<QStringList>(complexFilePaths,
                                                                       toLocalFile);

    if (!simpleFilePathStrings.isEmpty()) {
        if (targetDirPath.isEmpty()) {
            addResources(simpleFilePathStrings);
        } else {
            AddFilesResult result = ModelNodeOperations::addFilesToProject(simpleFilePathStrings,
                                                                           targetDirPath);
            if (result == AddFilesResult::Failed) {
                Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"),
                                                      tr("Could not add %1 to project.")
                                                          .arg(simpleFilePathStrings.join(' ')));
            }
        }
    }

    if (!complexFilePathStrings.empty())
        addResources(complexFilePathStrings);
}

QSet<QString> AssetsLibraryWidget::supportedAssetSuffixes(bool complex)
{
    const QList<AddResourceHandler> handlers = QmlDesignerPlugin::instance()->viewManager()
                                                   .designerActionManager().addResourceHandler();

    QSet<QString> suffixes;
    for (const AddResourceHandler &handler : handlers) {
        if (AssetsLibraryModel::supportedSuffixes().contains(handler.filter) != complex)
            suffixes.insert(handler.filter);
    }

    return suffixes;
}

void AssetsLibraryWidget::setModel(Model *model)
{
    m_model = model;
}

QString AssetsLibraryWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/itemLibraryQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/itemLibraryQmlSources").toString();
}

void AssetsLibraryWidget::clearSearchFilter()
{
    QMetaObject::invokeMethod(m_assetsWidget->rootObject(), "clearSearchFilter");
}

void AssetsLibraryWidget::reloadQmlSource()
{
    const QString assetsQmlPath = qmlSourcesPath() + "/Assets.qml";
    QTC_ASSERT(QFileInfo::exists(assetsQmlPath), return);
    m_assetsWidget->engine()->clearComponentCache();
    m_assetsWidget->setSource(QUrl::fromLocalFile(assetsQmlPath));
}

void AssetsLibraryWidget::updateSearch()
{
    m_assetsModel->setSearchText(m_filterText);
}

void AssetsLibraryWidget::setResourcePath(const QString &resourcePath)
{
    m_assetsModel->setRootPath(resourcePath);
    updateSearch();
}

void AssetsLibraryWidget::startDragAsset(const QStringList &assetPaths, const QPointF &mousePos)
{
    // Actual drag is created after mouse has moved to avoid a QDrag bug that causes drag to stay
    // active (and blocks mouse release) if mouse is released at the same spot of the drag start.
    m_assetsToDrag = assetPaths;
    m_dragStartPoint = mousePos.toPoint();
}

QPair<QString, QByteArray> AssetsLibraryWidget::getAssetTypeAndData(const QString &assetPath)
{
    QString suffix = "*." + assetPath.split('.').last().toLower();
    if (!suffix.isEmpty()) {
        if (AssetsLibraryModel::supportedImageSuffixes().contains(suffix)) {
            // Data: Image format (suffix)
            return {Constants::MIME_TYPE_ASSET_IMAGE, suffix.toUtf8()};
        } else if (AssetsLibraryModel::supportedFontSuffixes().contains(suffix)) {
            // Data: Font family name
            QRawFont font(assetPath, 10);
            QString fontFamily = font.isValid() ? font.familyName() : "";
            return {Constants::MIME_TYPE_ASSET_FONT, fontFamily.toUtf8()};
        } else if (AssetsLibraryModel::supportedShaderSuffixes().contains(suffix)) {
            // Data: shader type, frament (f) or vertex (v)
            return {Constants::MIME_TYPE_ASSET_SHADER,
                AssetsLibraryModel::supportedFragmentShaderSuffixes().contains(suffix) ? "f" : "v"};
        } else if (AssetsLibraryModel::supportedAudioSuffixes().contains(suffix)) {
            // No extra data for sounds
            return {Constants::MIME_TYPE_ASSET_SOUND, {}};
        } else if (AssetsLibraryModel::supportedVideoSuffixes().contains(suffix)) {
            // No extra data for videos
            return {Constants::MIME_TYPE_ASSET_VIDEO, {}};
        } else if (AssetsLibraryModel::supportedTexture3DSuffixes().contains(suffix)) {
            // Data: Image format (suffix)
            return {Constants::MIME_TYPE_ASSET_TEXTURE3D, suffix.toUtf8()};
        }
    }
    return {};
}

static QHash<QByteArray, QStringList> allImageFormats()
{
    const QList<QByteArray> mimeTypes = QImageReader::supportedMimeTypes();
    auto transformer = [](const QByteArray& format) -> QString { return QString("*.") + format; };
    QHash<QByteArray, QStringList> imageFormats;
    for (const auto &mimeType : mimeTypes)
        imageFormats.insert(mimeType, Utils::transform(QImageReader::imageFormatsForMimeType(mimeType), transformer));
    imageFormats.insert("image/vnd.radiance", {"*.hdr"});
    imageFormats.insert("image/ktx", {"*.ktx"});

    return imageFormats;
}

void AssetsLibraryWidget::addResources(const QStringList &files)
{
    clearSearchFilter();

    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();

    QTC_ASSERT(document, return);

    const QList<AddResourceHandler> handlers = QmlDesignerPlugin::instance()->viewManager()
                                                   .designerActionManager().addResourceHandler();

    QStringList fileNames = files;
    if (fileNames.isEmpty()) { // if no files, show the "add assets" dialog
        QMultiMap<QString, QString> map;
        QHash<QString, int> priorities;
        for (const AddResourceHandler &handler : handlers) {
            map.insert(handler.category, handler.filter);
            priorities.insert(handler.category, handler.piority);
        }

        QStringList sortedKeys = map.uniqueKeys();
        Utils::sort(sortedKeys, [&priorities](const QString &first, const QString &second) {
            return priorities.value(first) < priorities.value(second);
        });

        QStringList filters { tr("All Files (%1)").arg("*.*") };
        QString filterTemplate = "%1 (%2)";
        for (const QString &key : qAsConst(sortedKeys)) {
            const QStringList values = map.values(key);
            if (values.contains("*.png")) { // Avoid long filter for images by splitting
                const QHash<QByteArray, QStringList> imageFormats = allImageFormats();
                QHash<QByteArray, QStringList>::const_iterator i = imageFormats.constBegin();
                while (i != imageFormats.constEnd()) {
                    filters.append(filterTemplate.arg(key + QString::fromLatin1(i.key()), i.value().join(' ')));
                    ++i;
                }
            } else {
                filters.append(filterTemplate.arg(key, values.join(' ')));
            }
        }

        static QString lastDir;
        const QString currentDir = lastDir.isEmpty() ? document->fileName().parentDir().toString() : lastDir;

        fileNames = QFileDialog::getOpenFileNames(Core::ICore::dialogParent(),
                                                  tr("Add Assets"),
                                                  currentDir,
                                                  filters.join(";;"));

        if (!fileNames.isEmpty())
            lastDir = QFileInfo(fileNames.first()).absolutePath();
    }

    QHash<QString, QString> filterToCategory;
    QHash<QString, AddResourceOperation> categoryToOperation;
    for (const AddResourceHandler &handler : handlers) {
        filterToCategory.insert(handler.filter, handler.category);
        categoryToOperation.insert(handler.category, handler.operation);
    }

    QMultiMap<QString, QString> categoryFileNames; // filenames grouped by category

    for (const QString &fileName : qAsConst(fileNames)) {
        const QString suffix = "*." + QFileInfo(fileName).suffix().toLower();
        const QString category = filterToCategory.value(suffix);
        categoryFileNames.insert(category, fileName);
    }

    for (const QString &category : categoryFileNames.uniqueKeys()) {
        QStringList fileNames = categoryFileNames.values(category);
        AddResourceOperation operation = categoryToOperation.value(category);
        QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_RESOURCE_IMPORTED + category);
        if (operation) {
            AddFilesResult result = operation(fileNames,
                                              document->fileName().parentDir().toString());
            if (result == AddFilesResult::Failed) {
                Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"),
                                                      tr("Could not add %1 to project.")
                                                          .arg(fileNames.join(' ')));
            }
        } else {
            Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"),
                                                  tr("Could not add %1 to project. Unsupported file format.")
                                                      .arg(fileNames.join(' ')));
        }
    }
}

} // namespace QmlDesigner
