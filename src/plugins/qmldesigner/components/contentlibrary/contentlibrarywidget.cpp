// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarywidget.h"

#include "contentlibrarybundleimporter.h"
#include "contentlibraryeffectsmodel.h"
#include "contentlibraryiconprovider.h"
#include "contentlibraryitem.h"
#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialsmodel.h"
#include "contentlibrarytexture.h"
#include "contentlibrarytexturesmodel.h"
#include "contentlibraryusermodel.h"

#include <qmldesignerutils/filedownloader.h>
#include <qmldesignerutils/fileextractor.h>
#include <qmldesignerutils/multifiledownloader.h>

#include <coreplugin/icore.h>
#include <designerpaths.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <studioquickwidget.h>
#include <theme.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QMouseEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWidget>
#include <QRegularExpression>
#include <QShortcut>
#include <QVBoxLayout>

namespace QmlDesigner {

constexpr int TextureBundleMetadataVersion = 1;

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
        if (obj == m_quickWidget->quickWidget())
            QMetaObject::invokeMethod(m_quickWidget->rootObject(), "closeContextMenu");
    } else if (event->type() == QMouseEvent::MouseMove) {
        DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
        QTC_ASSERT(document, return false);
        Model *model = document->currentModel();
        QTC_ASSERT(model, return false);

        if (m_itemToDrag) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPos() - m_dragStartPoint).manhattanLength() > 20) {
                QByteArray data;
                auto mimeData = std::make_unique<QMimeData>();
                QDataStream stream(&data, QIODevice::WriteOnly);
                stream << m_itemToDrag->type();
                mimeData->setData(Constants::MIME_TYPE_BUNDLE_ITEM, data);

                emit bundleItemDragStarted(m_itemToDrag);
                const QString iconPath = m_itemToDrag->icon().toLocalFile();
                m_itemToDrag = nullptr;
                model->startDrag(std::move(mimeData), iconPath, this);
            }
        } else if (m_materialToDrag) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPosition().toPoint() - m_dragStartPoint).manhattanLength() > 20
                && m_materialsModel->isMaterialDownloaded(m_materialToDrag)) {
                QByteArray data;
                auto mimeData = std::make_unique<QMimeData>();
                QDataStream stream(&data, QIODevice::WriteOnly);
                stream << m_materialToDrag->type();
                mimeData->setData(Constants::MIME_TYPE_BUNDLE_MATERIAL, data);
                mimeData->removeFormat("text/plain");

                emit bundleMaterialDragStarted(m_materialToDrag);
                const QString iconPath = m_materialToDrag->icon().toLocalFile();
                m_materialToDrag = nullptr;
                model->startDrag(std::move(mimeData), iconPath, this);
            }
        } else if (m_textureToDrag) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPosition().toPoint() - m_dragStartPoint).manhattanLength() > 20
                && m_textureToDrag->isDownloaded()) {
                auto mimeData = std::make_unique<QMimeData>();
                mimeData->setData(Constants::MIME_TYPE_BUNDLE_TEXTURE,
                                  {m_textureToDrag->texturePath().toUtf8()});

                // Allows standard file drag-n-drop. As of now needed to drop on Assets view
                mimeData->setUrls({QUrl::fromLocalFile(m_textureToDrag->texturePath())});

                emit bundleTextureDragStarted(m_textureToDrag);
                const QString iconPath = m_textureToDrag->icon().toLocalFile();
                m_textureToDrag = nullptr;
                model->startDrag(std::move(mimeData), iconPath, this);
            }
        }
    } else if (event->type() == QMouseEvent::MouseButtonRelease) {
        m_itemToDrag = nullptr;
        m_materialToDrag = nullptr;
        m_textureToDrag = nullptr;
        setIsDragging(false);
    }

    return QObject::eventFilter(obj, event);
}

ContentLibraryWidget::ContentLibraryWidget()
    : m_iconProvider(Utils::makeUniqueObjectPtr<ContentLibraryIconProvider>())
    , m_quickWidget(Utils::makeUniqueObjectPtr<StudioQuickWidget>(this))
    , m_materialsModel(new ContentLibraryMaterialsModel(this))
    , m_texturesModel(new ContentLibraryTexturesModel("Textures", this))
    , m_environmentsModel(new ContentLibraryTexturesModel("Environments", this))
    , m_effectsModel(new ContentLibraryEffectsModel(this))
    , m_userModel(new ContentLibraryUserModel(this))
{
    qmlRegisterType<QmlDesigner::FileDownloader>("WebFetcher", 1, 0, "FileDownloader");
    qmlRegisterType<QmlDesigner::FileExtractor>("WebFetcher", 1, 0, "FileExtractor");

    setWindowTitle(tr("Content Library", "Title of content library widget"));
    setMinimumWidth(120);

    m_quickWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_CONTENT_LIBRARY);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->engine()->addImageProvider("contentlibrary", m_iconProvider.get());
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->setClearColor(Theme::getColor(Theme::Color::DSpanelBackground));

    m_textureBundleUrl = QmlDesignerPlugin::settings()
                    .value(DesignerSettingsKey::DOWNLOADABLE_BUNDLES_URL).toString() + "/textures";

    m_bundlePath = Paths::bundlesPathSetting();

    loadTextureBundles();

    Theme::setupTheme(m_quickWidget->engine());
    m_quickWidget->quickWidget()->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_quickWidget.get());

    updateSearch();

    setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F11), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &ContentLibraryWidget::reloadQmlSource);

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_CONTENTLIBRARY_TIME);

    auto map = m_quickWidget->registerPropertyMap("ContentLibraryBackend");

    map->setProperties({{"rootView",          QVariant::fromValue(this)},
                        {"materialsModel",    QVariant::fromValue(m_materialsModel.data())},
                        {"texturesModel",     QVariant::fromValue(m_texturesModel.data())},
                        {"environmentsModel", QVariant::fromValue(m_environmentsModel.data())},
                        {"effectsModel",      QVariant::fromValue(m_effectsModel.data())},
                        {"userModel",         QVariant::fromValue(m_userModel.data())}});

    reloadQmlSource();
    createImporter();
}

ContentLibraryWidget::~ContentLibraryWidget()
{
}

void ContentLibraryWidget::createImporter()
{
    m_importer = new ContentLibraryBundleImporter();
#ifdef QDS_USE_PROJECTSTORAGE
    connect(m_importer,
            &ContentLibraryBundleImporter::importFinished,
            this,
            [&](const QmlDesigner::TypeName &typeName, const QString &bundleId) {
                setImporterRunning(false);
                if (typeName.size())
                    updateImportedState(bundleId);
            });
#else
    connect(m_importer,
            &ContentLibraryBundleImporter::importFinished,
            this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo, const QString &bundleId) {
                setImporterRunning(false);
                if (metaInfo.isValid())
                    updateImportedState(bundleId);
            });
#endif

    connect(m_importer, &ContentLibraryBundleImporter::unimportFinished, this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo, const QString &bundleId) {
                Q_UNUSED(metaInfo)
                setImporterRunning(false);
                updateImportedState(bundleId);
    });
}

ContentLibraryIconProvider *ContentLibraryWidget::iconProvider() const
{
    return m_iconProvider.get();
}

void ContentLibraryWidget::showTab(TabIndex tabIndex)
{
    emit requestTab(int(tabIndex));
}

void ContentLibraryWidget::updateImportedState(const QString &bundleId)
{
    if (!m_importer)
        return;

    Utils::FilePath bundlePath = m_importer->resolveBundleImportPath(bundleId);

    QStringList importedItems;
    if (bundlePath.exists()) {
        importedItems = transform(bundlePath.dirEntries({{"*.qml"}, QDir::Files}),
                                  [](const Utils::FilePath &f) { return f.baseName(); });
    }

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    if (bundleId == compUtils.materialsBundleId())
        m_materialsModel->updateImportedState(importedItems);
    else if (bundleId == compUtils.effectsBundleId())
        m_effectsModel->updateImportedState(importedItems);
    else
        m_userModel->updateImportedState(importedItems, bundleId);
}

ContentLibraryBundleImporter *ContentLibraryWidget::importer() const
{
    return m_importer;
}

QVariantMap ContentLibraryWidget::readTextureBundleJson()
{
    QVariantMap jsonData;
    QFile jsonFile(m_bundlePath + "/texture_bundle.json");
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
        jsonData = QJsonDocument::fromJson(jsonFile.readAll()).toVariant().toMap();

    int version = jsonData["version"].toInt();
    if (version > TextureBundleMetadataVersion) {
        qWarning() << "Unrecognized texture metadata file version: " << version;
        return {};
    }

    return jsonData;
}

void ContentLibraryWidget::loadTextureBundles()
{
    QDir bundleDir{m_bundlePath};

    if (fetchTextureBundleJson(bundleDir) && fetchTextureBundleIcons(bundleDir))
        populateTextureBundleModels();
}

std::tuple<QVariantMap, QVariantMap, QVariantMap> ContentLibraryWidget::compareTextureMetaFiles(
    const QString &existingMetaFilePath, const QString downloadedMetaFilePath)
{
    QVariantMap existingMeta;
    QFile existingFile(existingMetaFilePath);
    if (existingFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
        existingMeta = QJsonDocument::fromJson(existingFile.readAll()).toVariant().toMap();

    QVariantMap downloadedMeta;
    QFile downloadedFile(downloadedMetaFilePath);
    if (downloadedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
        downloadedMeta = QJsonDocument::fromJson(downloadedFile.readAll()).toVariant().toMap();

    int existingVersion = existingMeta["version"].toInt();
    int downloadedVersion = downloadedMeta["version"].toInt();

    if (existingVersion != downloadedVersion) {
        qWarning() << "We're not comparing local vs downloaded textures metadata because they are "
                      "of different versions";
        return {};
    }

    QVariantMap existingItems = existingMeta["image_items"].toMap();
    QVariantMap downloadedItems = downloadedMeta["image_items"].toMap();

    QStringList existingKeys = existingItems.keys();
    QStringList downloadedKeys = downloadedItems.keys();

    QSet<QString> existing(existingKeys.cbegin(), existingKeys.cend());
    QSet<QString> downloaded(downloadedKeys.cbegin(), downloadedKeys.cend());

    const QSet<QString> newFiles = downloaded - existing;
    const QSet<QString> commonFiles = downloaded & existing;

    QVariantMap modifiedFileEntries;

    for (const QString &file: commonFiles) {
        QString existingCsum = existingItems[file].toMap()["checksum"].toString();
        QString downloadedCsum = downloadedItems[file].toMap()["checksum"].toString();

        if (existingCsum != downloadedCsum)
            modifiedFileEntries[file] = downloadedItems[file];
    }

    QVariantMap newFileEntries;
    for (const QString &path: newFiles)
        newFileEntries[path] = downloadedItems[path];

    return std::make_tuple(existingItems, newFileEntries, modifiedFileEntries);
}

void ContentLibraryWidget::fetchNewTextureIcons(const QVariantMap &existingFiles,
                                                const QVariantMap &newFiles,
                                                const QString &existingMetaFilePath,
                                                const QDir &bundleDir)
{
    QStringList fileList = Utils::transform<QList>(newFiles.keys(), [](const QString &file) -> QString {
        return file + ".png";
    });

    auto multidownloader = new MultiFileDownloader(this);
    multidownloader->setBaseUrl(QString(m_textureBundleUrl + "/icons"));
    multidownloader->setFiles(fileList);
    multidownloader->setTargetDirPath(m_bundlePath + "/TextureBundleIcons");

    auto downloader = new FileDownloader(this);
    downloader->setDownloadEnabled(true);
    downloader->setProbeUrl(false);

    downloader->setUrl(multidownloader->nextUrl());
    downloader->setTargetFilePath(multidownloader->nextTargetPath());

    QObject::connect(multidownloader, &MultiFileDownloader::nextUrlChanged, downloader, [=]() {
        downloader->setUrl(multidownloader->nextUrl());
    });

    QObject::connect(multidownloader, &MultiFileDownloader::nextTargetPathChanged, downloader, [=]() {
        downloader->setTargetFilePath(multidownloader->nextTargetPath());
    });

    multidownloader->setDownloader(downloader);

    QVariantMap files = existingFiles;
    files.insert(newFiles);

    QObject::connect(multidownloader, &MultiFileDownloader::finishedChanged, this,
                     [multidownloader, files, existingMetaFilePath, this, bundleDir]() {
        multidownloader->deleteLater();

        QVariantMap newMap;
        newMap["version"] = TextureBundleMetadataVersion;
        newMap["image_items"] = files;

        QJsonObject jobj = QJsonObject::fromVariantMap(newMap);
        QJsonDocument doc(jobj);
        QByteArray data = doc.toJson();

        QFile existingFile(existingMetaFilePath);
        if (existingFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text)) {
            existingFile.write(data);
            existingFile.flush();
        }

        if (fetchTextureBundleIcons(bundleDir))
            populateTextureBundleModels();
    });

    multidownloader->start();
}

QStringList ContentLibraryWidget::saveNewTextures(const QDir &bundleDir, const QStringList &newFiles)
{
    int newFileExpirationDays = QmlDesignerPlugin::settings()
                                    .value(DesignerSettingsKey::CONTENT_LIBRARY_NEW_FLAG_EXPIRATION_DAYS)
                                    .toInt();

    int newFileExpirationSecs = newFileExpirationDays * 24 * 3600;

    QString newFilesPath = bundleDir.filePath(".new_textures_on_server.json");

    QFile jsonFile(newFilesPath);
    if (jsonFile.exists()) {
        jsonFile.open(QFile::ReadOnly | QFile::Text);

        qint64 now = QDateTime::currentSecsSinceEpoch();
        QByteArray existingData = jsonFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(existingData);
        jsonFile.close();

        QJsonObject mainObj = doc.object();
        if (mainObj.value("version").toInt() > TextureBundleMetadataVersion) {
            qDebug() << "Existing version of cached 'New Items' does not have a known version.";

            // TODO: do simple save new file
            return newFiles;
        }

        QJsonValue jsonValue = mainObj.value("image_items");
        QJsonArray imageItems = jsonValue.toArray();

        // remove those files that are older than N days (configurable via QSettings)
        imageItems = Utils::filtered(imageItems, [newFileExpirationSecs, now](const QJsonValue &v) {
            qint64 time = v["time"].toInt();
            if (now - time >= newFileExpirationSecs)
                return false;

            return true;
        });

        QStringList pruned = Utils::transform<QStringList>(imageItems, [](const QJsonValue &value) -> QString {
            return value.toObject()["file"].toString();
        });

        // filter out files from newFiles that already exist in the document

        QStringList newFilesNow = Utils::filtered(newFiles, [&imageItems](const QString &file) {
            bool contains = Utils::anyOf(imageItems, [file](const QJsonValue &v) {
                if (!v.isObject())
                    return false;

                QJsonObject o = v.toObject();
                if (!o.contains("file"))
                    return false;

                bool hasFile = (o["file"] == file);
                return hasFile;
            });
            return !contains;
        });

        // add the filtered out files to the doc.
        for (const QString &file: newFilesNow) {
            QJsonObject obj({{"file", file}, {"time", now}});
            imageItems.push_back(obj);
        }

        mainObj["image_items"] = imageItems;

        // save the json file.
        doc.setObject(mainObj);
        QByteArray data = doc.toJson();

        jsonFile.open(QFile::WriteOnly | QFile::Text);
        jsonFile.write(data);
        jsonFile.close();

        return newFilesNow + pruned;
    } else {
        qint64 now = QDateTime::currentSecsSinceEpoch();

        QJsonArray texturesFoundNow = Utils::transform<QJsonArray>(newFiles, [now](const QString &file) {
            QJsonObject obj({{"file", file}, {"time", now}});
            return QJsonValue(obj);
        });

        QVariantMap varMap;
        varMap["version"] = TextureBundleMetadataVersion;
        varMap["image_items"] = texturesFoundNow;

        QJsonObject mainObj({{"version", TextureBundleMetadataVersion},
                             {"image_items", texturesFoundNow}});

        QJsonDocument doc(mainObj);
        QByteArray data = doc.toJson();

        jsonFile.open(QFile::WriteOnly | QFile::Text);
        jsonFile.write(data);
        jsonFile.close();

        return newFiles;
    }
}

bool ContentLibraryWidget::fetchTextureBundleJson(const QDir &bundleDir)
{
    QString filePath = bundleDir.filePath("texture_bundle.json");

    QFileInfo fi(filePath);
    bool jsonFileExists = fi.exists() && fi.size() > 0;

    QString bundleZipUrl = m_textureBundleUrl + "/texture_bundle.zip";
    FileDownloader *downloader = new FileDownloader(this);
    downloader->setUrl(bundleZipUrl);
    downloader->setProbeUrl(false);
    downloader->setDownloadEnabled(true);
    downloader->start();

    QObject::connect(downloader, &FileDownloader::downloadFailed, this,
                     [this, jsonFileExists, bundleDir] {
        if (jsonFileExists) {
            if (fetchTextureBundleIcons(bundleDir))
                populateTextureBundleModels();
        }
    });

    QObject::connect(downloader, &FileDownloader::finishedChanged, this,
                     [this, downloader, bundleDir, jsonFileExists, filePath] {
        FileExtractor *extractor = new FileExtractor(this);
        extractor->setArchiveName(downloader->completeBaseName());
        extractor->setSourceFile(downloader->outputFile());
        if (!jsonFileExists)
            extractor->setTargetPath(bundleDir.absolutePath());

        extractor->setAlwaysCreateDir(false);
        extractor->setClearTargetPathContents(false);

        QObject::connect(extractor, &FileExtractor::finishedChanged, this,
                         [this, downloader, bundleDir, extractor, jsonFileExists, filePath] {
            downloader->deleteLater();
            extractor->deleteLater();

            if (jsonFileExists) {
                QVariantMap newFiles, existing;
                QVariantMap modifiedFilesEntries;

                std::tie(existing, newFiles, modifiedFilesEntries) =
                    compareTextureMetaFiles(filePath, extractor->targetPath() + "/texture_bundle.json");

                const QStringList newFilesKeys = newFiles.keys();
                const QStringList actualNewFiles = saveNewTextures(bundleDir, newFilesKeys);

                m_texturesModel->setModifiedFileEntries(modifiedFilesEntries);
                m_texturesModel->setNewFileEntries(actualNewFiles);

                m_environmentsModel->setModifiedFileEntries(modifiedFilesEntries);
                m_environmentsModel->setNewFileEntries(actualNewFiles);

                if (newFiles.count() > 0) {
                    fetchNewTextureIcons(existing, newFiles, filePath, bundleDir);
                    return;
                }
            }

            if (fetchTextureBundleIcons(bundleDir))
                populateTextureBundleModels();
        });

        extractor->extract();
    });

    return false;
}

void ContentLibraryWidget::populateTextureBundleModels()
{
    QVariantMap jsonData = readTextureBundleJson();

    QString bundleIconPath = m_bundlePath + "/TextureBundleIcons";

    m_texturesModel->loadTextureBundle(m_textureBundleUrl, bundleIconPath, jsonData);
    m_environmentsModel->loadTextureBundle(m_textureBundleUrl, bundleIconPath, jsonData);
}

bool ContentLibraryWidget::fetchTextureBundleIcons(const QDir &bundleDir)
{
    QString iconsPath = bundleDir.filePath("TextureBundleIcons");

    QDir iconsDir(iconsPath);
    if (iconsDir.exists() && !iconsDir.isEmpty())
        return true;

    QString zipFileUrl = m_textureBundleUrl + "/icons.zip";

    FileDownloader *downloader = new FileDownloader(this);
    downloader->setUrl(zipFileUrl);
    downloader->setProbeUrl(false);
    downloader->setDownloadEnabled(true);

    QObject::connect(downloader, &FileDownloader::finishedChanged, this,
                     [this, downloader, bundleDir] {
        FileExtractor *extractor = new FileExtractor(this);
        extractor->setArchiveName(downloader->completeBaseName());
        extractor->setSourceFile(downloader->outputFile());
        extractor->setTargetPath(bundleDir.absolutePath());
        extractor->setAlwaysCreateDir(false);
        extractor->setClearTargetPathContents(false);

        QObject::connect(extractor, &FileExtractor::finishedChanged, this,
                         [this, downloader, extractor] {
            downloader->deleteLater();
            extractor->deleteLater();
            populateTextureBundleModels();
        });

        extractor->extract();
    });

    downloader->start();
    return false;
}

void ContentLibraryWidget::markTextureUpdated(const QString &textureKey)
{
    static QRegularExpression re("([^/]+)/([^/]+)/.*");
    QString category = re.match(textureKey).captured(1);
    QString subcategory = re.match(textureKey).captured(2);

    QString checksumOnServer;
    if (category == "Textures")
        checksumOnServer = m_texturesModel->removeModifiedFileEntry(textureKey);
    else if (category == "Environments")
        checksumOnServer = m_environmentsModel->removeModifiedFileEntry(textureKey);

    QJsonObject metaDataObj;
    QFile jsonFile(m_bundlePath + "/texture_bundle.json");
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        metaDataObj = QJsonDocument::fromJson(jsonFile.readAll()).object();
        jsonFile.close();
    }

    QJsonObject imageItems = metaDataObj["image_items"].toObject();

    QJsonObject oldImageItem = imageItems[textureKey].toObject();
    oldImageItem["checksum"] = checksumOnServer;
    imageItems[textureKey] = oldImageItem;

    metaDataObj["image_items"] = imageItems;

    QJsonDocument outDoc(metaDataObj);
    QByteArray data = outDoc.toJson();

    QFile outFile(m_bundlePath + "/texture_bundle.json");
    if (outFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text)) {
        outFile.write(data);
        outFile.flush();
    }

    if (category == "Textures")
        m_texturesModel->markTextureHasNoUpdates(subcategory, textureKey);
    else if (category == "Environments")
        m_environmentsModel->markTextureHasNoUpdates(subcategory, textureKey);
}

QSize ContentLibraryWidget::sizeHint() const
{
    return {420, 420};
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
    m_effectsModel->updateIsEmpty();
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

bool ContentLibraryWidget::hasActive3DScene() const
{
    return m_hasActive3DScene;
}

void ContentLibraryWidget::setHasActive3DScene(bool b)
{
    if (m_hasActive3DScene == b)
        return;

    m_hasActive3DScene = b;
    emit hasActive3DSceneChanged();
}

bool ContentLibraryWidget::isQt6Project() const
{
    return m_isQt6Project;
}

void ContentLibraryWidget::setIsQt6Project(bool b)
{
    if (m_isQt6Project == b)
        return;

    m_isQt6Project = b;
    emit isQt6ProjectChanged();
}

bool ContentLibraryWidget::importerRunning() const
{
    return m_importerRunning;
}

void ContentLibraryWidget::setImporterRunning(bool b)
{
    if (m_importerRunning == b)
        return;

    m_importerRunning = b;
    emit importerRunningChanged();
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
    m_effectsModel->setSearchText(m_filterText);
    m_texturesModel->setSearchText(m_filterText);
    m_environmentsModel->setSearchText(m_filterText);
    m_userModel->setSearchText(m_filterText);
    m_quickWidget->update();
}

void ContentLibraryWidget::setIsDragging(bool val)
{
    if (m_isDragging != val) {
        m_isDragging = val;
        emit isDraggingChanged();
    }
}

void ContentLibraryWidget::startDragItem(QmlDesigner::ContentLibraryItem *item, const QPointF &mousePos)
{
    m_itemToDrag = item;
    m_dragStartPoint = mousePos.toPoint();
    setIsDragging(true);
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

    emit addTextureRequested(tex->texturePath(), AddTextureMode::Image);
}

void ContentLibraryWidget::addTexture(ContentLibraryTexture *tex)
{
    if (!tex->isDownloaded())
        return;

    emit addTextureRequested(tex->texturePath(), AddTextureMode::Texture);
}

void ContentLibraryWidget::addLightProbe(ContentLibraryTexture *tex)
{
    if (!tex->isDownloaded())
        return;

    emit addTextureRequested(tex->texturePath(), AddTextureMode::LightProbe);
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

QPointer<ContentLibraryEffectsModel> ContentLibraryWidget::effectsModel() const
{
    return m_effectsModel;
}

QPointer<ContentLibraryUserModel> ContentLibraryWidget::userModel() const
{
    return m_userModel;
}

bool ContentLibraryWidget::hasModelSelection() const
{
    return m_hasModelSelection;
}

void ContentLibraryWidget::setHasModelSelection(bool b)
{
    if (b == m_hasModelSelection)
        return;

    m_hasModelSelection = b;
    emit hasModelSelectionChanged();
}

} // namespace QmlDesigner
