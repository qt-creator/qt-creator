/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "customfilesystemmodel.h"

#include <synchronousimagecache.h>
#include <theme.h>
#include <hdrimage.h>

#include <utils/filesystemwatcher.h>

#include <QDir>
#include <QDirIterator>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QFont>
#include <QImageReader>
#include <QPainter>
#include <QRawFont>
#include <qmath.h>
#include <condition_variable>
#include <mutex>

namespace QmlDesigner {

static const QStringList &supportedImageSuffixes()
{
    static QStringList retList;
    if (retList.isEmpty()) {
        const QList<QByteArray> suffixes = QImageReader::supportedImageFormats();
        for (const QByteArray &suffix : suffixes)
            retList.append(QString::fromUtf8(suffix));
    }
    return retList;
}

static const QStringList &supportedFragmentShaderSuffixes()
{
    static const QStringList retList {"frag", "glsl", "glslf", "fsh"};
    return retList;
}

static const QStringList &supportedShaderSuffixes()
{
    static const QStringList retList {"frag", "vert",
                                      "glsl", "glslv", "glslf",
                                      "vsh", "fsh"};
    return retList;
}

static const QStringList &supportedFontSuffixes()
{
    static const QStringList retList {"ttf", "otf"};
    return retList;
}

static const QStringList &supportedAudioSuffixes()
{
    static const QStringList retList {"wav"};
    return retList;
}

static const QStringList &supportedTexture3DSuffixes()
{
    // These are file types only supported by 3D textures
    static QStringList retList {"hdr"};
    return retList;
}

static QPixmap defaultPixmapForType(const QString &type, const QSize &size)
{
    return QPixmap(QStringLiteral(":/ItemLibrary/images/asset_%1_%2.png").arg(type).arg(size.width()));
}

static QPixmap texturePixmap(const QString &fileName)
{
    return HdrImage{fileName}.toPixmap();
}

QString fontFamily(const QFileInfo &info)
{
    QRawFont font(info.absoluteFilePath(), 10);
    if (font.isValid())
        return font.familyName();
    return {};
}

class ItemLibraryFileIconProvider : public QFileIconProvider
{
public:
    ItemLibraryFileIconProvider(SynchronousImageCache &fontImageCache,
                                QHash<QString, QPair<QDateTime, QIcon>> &iconCache)
        : QFileIconProvider()
        , m_fontImageCache(fontImageCache)
        , m_iconCache(iconCache)
    {
    }

    QIcon icon( const QFileInfo & info ) const override
    {
        const QString filePath = info.absoluteFilePath();
        QPair<QDateTime, QIcon> &cachedIcon = m_iconCache[filePath];
        if (!cachedIcon.second.isNull() && cachedIcon.first == info.lastModified())
            return cachedIcon.second;

        QIcon icon;
        const QString suffix = info.suffix().toLower();

        // Provide icon depending on suffix
        QPixmap origPixmap;

        if (supportedFontSuffixes().contains(suffix))
            return generateFontIcons(filePath);
        else if (supportedImageSuffixes().contains(suffix))
            origPixmap.load(filePath);
        else if (supportedTexture3DSuffixes().contains(suffix))
            origPixmap = texturePixmap(filePath);

        for (auto iconSize : iconSizes) {
            QPixmap pixmap = origPixmap;
            if (pixmap.isNull()) {
                if (supportedAudioSuffixes().contains(suffix))
                    pixmap = defaultPixmapForType("sound", iconSize);
                else if (supportedShaderSuffixes().contains(suffix))
                    pixmap = defaultPixmapForType("shader", iconSize);
                if (pixmap.isNull())
                    return QFileIconProvider::icon(info);
            }

            if ((pixmap.width() > iconSize.width()) || (pixmap.height() > iconSize.height()))
                pixmap = pixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            icon.addPixmap(pixmap);
        }

        cachedIcon.first = info.lastModified();
        cachedIcon.second = icon;
        return icon;
    }

    QIcon generateFontIcons(const QString &filePath) const
    {
        return m_fontImageCache.icon(
            filePath,
            {},
            ImageCache::FontCollectorSizesAuxiliaryData{Utils::span{iconSizes},
                                                        Theme::getColor(Theme::DStextColor).name(),
                                                        "Abc"});
    }

private:
    // Generated icon sizes should contain all ItemLibraryResourceView needed icon sizes, and their
    // x2 versions for HDPI sceens
    std::vector<QSize> iconSizes = {{384, 384},
                                    {192, 192}, // Large
                                    {256, 256},
                                    {128, 128}, // Drag
                                    {96, 96},   // Medium
                                    {48, 48},   // Small
                                    {64, 64},
                                    {32, 32}}; // List

    SynchronousImageCache &m_fontImageCache;
    QHash<QString, QPair<QDateTime, QIcon>> &m_iconCache;
};

CustomFileSystemModel::CustomFileSystemModel(SynchronousImageCache &fontImageCache, QObject *parent)
    : QAbstractListModel(parent)
    , m_fileSystemModel(new QFileSystemModel(this))
    , m_fileSystemWatcher(new Utils::FileSystemWatcher(this))
    , m_fontImageCache(fontImageCache)
{
    m_updatePathTimer.setInterval(200);
    m_updatePathTimer.setSingleShot(true);
    m_updatePathTimer.callOnTimeout([this]() {
        updatePath(m_fileSystemModel->rootPath());
    });

    // If project directory contents change, or one of the asset files is modified, we must
    // reconstruct the model to update the icons
    connect(m_fileSystemWatcher, &Utils::FileSystemWatcher::directoryChanged, [this] {
        m_updatePathTimer.start();
    });
    connect(m_fileSystemWatcher, &Utils::FileSystemWatcher::fileChanged, [this] {
        m_updatePathTimer.start();
    });
}

void CustomFileSystemModel::setFilter(QDir::Filters)
{

}

bool filterMetaIcons(const QString &fileName)
{

    QFileInfo info(fileName);

    if (info.dir().path().split("/").contains("designer")) {

        QDir currentDir = info.dir();

        int i = 0;
        while (!currentDir.isRoot() && i < 3) {
            if (currentDir.dirName() == "designer") {
                if (!currentDir.entryList({"*.metainfo"}).isEmpty())
                    return false;
            }

            currentDir.cdUp();
            ++i;
        }

        if (info.dir().dirName() == "designer")
            return false;
    }

    return true;
}

QModelIndex CustomFileSystemModel::setRootPath(const QString &newPath)
{
    if (m_fileSystemModel->rootPath() == newPath)
        return QAbstractListModel::index(0, 0);

    return updatePath(newPath);
}

QVariant CustomFileSystemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole)
        return fileInfo(index).filePath();

    if (role == Qt::FontRole) {
        QFont font = m_fileSystemModel->data(fileSystemModelIndex(index), role).value<QFont>();
        font.setPixelSize(Theme::instance()->smallFontPixelSize());
        return font;
    }


    return m_fileSystemModel->data(fileSystemModelIndex(index), role);
}

int CustomFileSystemModel::rowCount(const QModelIndex &) const
{
    return m_files.count();
}

int CustomFileSystemModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QModelIndex CustomFileSystemModel::indexForPath(const QString &path, int /*column*/) const
{
    return QAbstractListModel::index(m_files.indexOf(path), 0);
}

QIcon CustomFileSystemModel::fileIcon(const QModelIndex &index) const
{
    return m_fileSystemModel->fileIcon(fileSystemModelIndex(index));
}

QString CustomFileSystemModel::fileName(const QModelIndex &index) const
{
    return m_fileSystemModel->fileName(fileSystemModelIndex(index));
}

QFileInfo CustomFileSystemModel::fileInfo(const QModelIndex &index) const
{
    return m_fileSystemModel->fileInfo(fileSystemModelIndex(index));
}

Qt::ItemFlags CustomFileSystemModel::flags(const QModelIndex &index) const
{
    return m_fileSystemModel->flags (fileSystemModelIndex(index));
}

void CustomFileSystemModel::setSearchFilter(const QString &nameFilterList)
{
    m_searchFilter = nameFilterList;
    setRootPath(m_fileSystemModel->rootPath());
}

QPair<QString, QByteArray> CustomFileSystemModel::resourceTypeAndData(const QModelIndex &index) const
{
    QFileInfo fi = fileInfo(index);
    QString suffix = fi.suffix().toLower();
    if (!suffix.isEmpty()) {
        if (supportedImageSuffixes().contains(suffix)) {
            // Data: Image format (suffix)
            return {"application/vnd.bauhaus.libraryresource.image", suffix.toUtf8()};
        } else if (supportedFontSuffixes().contains(suffix)) {
            // Data: Font family name
            return {"application/vnd.bauhaus.libraryresource.font", fontFamily(fi).toUtf8()};
        } else if (supportedShaderSuffixes().contains(suffix)) {
            // Data: shader type, frament (f) or vertex (v)
            return {"application/vnd.bauhaus.libraryresource.shader",
                supportedFragmentShaderSuffixes().contains(suffix) ? "f" : "v"};
        } else if (supportedAudioSuffixes().contains(suffix)) {
            // No extra data for sounds
            return {"application/vnd.bauhaus.libraryresource.sound", {}};
        } else if (supportedTexture3DSuffixes().contains(suffix)) {
            // Data: Image format (suffix)
            return {"application/vnd.bauhaus.libraryresource.texture3d", suffix.toUtf8()};
        }
    }
    return {};
}

const QSet<QString> &CustomFileSystemModel::supportedSuffixes() const
{
    static QSet<QString> allSuffixes;
    if (allSuffixes.isEmpty()) {
        auto insertSuffixes = [](const QStringList &suffixes) {
            for (const auto &suffix : suffixes)
                allSuffixes.insert(suffix);
        };
        insertSuffixes(supportedImageSuffixes());
        insertSuffixes(supportedShaderSuffixes());
        insertSuffixes(supportedFontSuffixes());
        insertSuffixes(supportedAudioSuffixes());
        insertSuffixes(supportedTexture3DSuffixes());
    }
    return allSuffixes;
}

const QSet<QString> &CustomFileSystemModel::previewableSuffixes() const
{
    static QSet<QString> previewableSuffixes;
    if (previewableSuffixes.isEmpty()) {
        auto insertSuffixes = [](const QStringList &suffixes) {
            for (const auto &suffix : suffixes)
                previewableSuffixes.insert(suffix);
        };
        insertSuffixes(supportedFontSuffixes());
    }
    return previewableSuffixes;

}

void CustomFileSystemModel::appendIfNotFiltered(const QString &file)
{
    if (filterMetaIcons(file))
        m_files.append(file);
}

QModelIndex CustomFileSystemModel::updatePath(const QString &newPath)
{
    beginResetModel();

    // We must recreate icon provider to ensure modified icons are recreated
    auto newProvider = new ItemLibraryFileIconProvider(m_fontImageCache, m_iconCache);
    m_fileSystemModel->setIconProvider(newProvider);
    delete m_fileIconProvider;
    m_fileIconProvider = newProvider;

    m_fileSystemModel->setRootPath(newPath);

    m_fileSystemWatcher->removeDirectories(m_fileSystemWatcher->directories());
    m_fileSystemWatcher->removeFiles(m_fileSystemWatcher->files());

    m_fileSystemWatcher->addDirectory(newPath, Utils::FileSystemWatcher::WatchAllChanges);

    QStringList nameFilterList;

    const QString searchFilter = m_searchFilter;

    if (searchFilter.contains(QLatin1Char('.'))) {
        nameFilterList.append(QString(QStringLiteral("*%1*")).arg(searchFilter));
    } else {
        const QString filterTemplate("*%1*.%2");
        auto appendFilters = [&](const QStringList &suffixes) {
            for (const QString &ext : suffixes) {
                nameFilterList.append(filterTemplate.arg(searchFilter, ext));
                nameFilterList.append(filterTemplate.arg(searchFilter, ext.toUpper()));
            }
        };
        appendFilters(supportedImageSuffixes());
        appendFilters(supportedShaderSuffixes());
        appendFilters(supportedFontSuffixes());
        appendFilters(supportedAudioSuffixes());
        appendFilters(supportedTexture3DSuffixes());
    }

    m_files.clear();

    QDirIterator fileIterator(newPath, nameFilterList, QDir::Files, QDirIterator::Subdirectories);

    while (fileIterator.hasNext())
        appendIfNotFiltered(fileIterator.next());

    QDirIterator dirIterator(newPath, {}, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        const QString entry = dirIterator.next();
        QFileInfo fi{entry};
        if (fi.isDir())
            m_fileSystemWatcher->addDirectory(entry, Utils::FileSystemWatcher::WatchAllChanges);
        else if (supportedSuffixes().contains(fi.suffix()))
            m_fileSystemWatcher->addFile(entry, Utils::FileSystemWatcher::WatchAllChanges);
    }

    endResetModel();

    return QAbstractListModel::index(0, 0);
}

QModelIndex CustomFileSystemModel::fileSystemModelIndex(const QModelIndex &index) const
{
    const int row = index.row();
    return m_fileSystemModel->index(m_files.at(row));
}

} //QmlDesigner
