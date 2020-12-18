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

#include <theme.h>

#include <utils/filesystemwatcher.h>

#include <QDir>
#include <QDirIterator>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QFont>
#include <QImageReader>
#include <QPainter>
#include <QRawFont>
#include <QGlyphRun>
#include <qmath.h>

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

static QPixmap generateFontImage(const QFileInfo &info, const QSize &size)
{
    static QHash<QString, QPixmap> fontImageCache;
    const QString file = info.absoluteFilePath();
    const QString key = QStringLiteral("%1@%2@%3").arg(file).arg(size.width()).arg(size.height());
    if (!fontImageCache.contains(key)) {
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);
        qreal pixelSize = size.width() / 2.;
        bool done = false;
        while (!done) {
            QRawFont font(file, pixelSize);
            if (!font.isValid())
                return pixmap;
            QGlyphRun gr;
            gr.setRawFont(font);
            QVector<quint32> indices = font.glyphIndexesForString("Abc");
            if (indices.isEmpty())
                return pixmap;
            const QVector<QPointF> advances = font.advancesForGlyphIndexes(indices);
            QVector<QPointF> positions;
            QPointF totalAdvance;
            for (const QPointF &advance : advances) {
                positions.append(totalAdvance);
                totalAdvance += advance;
            }

            gr.setGlyphIndexes(indices);
            gr.setPositions(positions);
            QRectF bounds = gr.boundingRect();
            if (bounds.width() <= 0 || bounds.height() <= 0)
                return pixmap;

            bounds.moveCenter({size.width() / 2., size.height() / 2.});

            // Bounding rectangle doesn't necessarily contain the entirety of glyphs for some
            // reason, so also check totalAdvance for overflow.
            qreal limitX = qMax(bounds.width(), totalAdvance.x());
            if (size.width() < limitX) {
                pixelSize = qreal(qFloor(pixelSize * size.width() / limitX));
                continue;
            }
            qreal limitY = qMax(bounds.height(), totalAdvance.y());
            if (size.height() < limitY) {
                pixelSize = qreal(qFloor(pixelSize * size.height() / limitY));
                continue;
            }

            QPainter painter(&pixmap);
            painter.setPen(Theme::getColor(Theme::DStextColor));
            painter.drawGlyphRun(bounds.bottomLeft(), gr);
            done = true;
        }
        fontImageCache[key] = pixmap;
    }
    return fontImageCache[key];
}

class ItemLibraryFileIconProvider : public QFileIconProvider
{
public:
    ItemLibraryFileIconProvider() = default;

    QIcon icon( const QFileInfo & info ) const override
    {
        QIcon icon;
        const QString suffix = info.suffix();

        for (auto iconSize : iconSizes) {
            // Provide icon depending on suffix
            QPixmap pixmap;

            if (supportedImageSuffixes().contains(suffix))
                pixmap.load(info.absoluteFilePath());
            else  if (supportedFontSuffixes().contains(suffix))
                pixmap = generateFontImage(info, iconSize);

            if (pixmap.isNull())
                return QFileIconProvider::icon(info);

            if ((pixmap.width() > iconSize.width()) || (pixmap.height() > iconSize.height()))
                pixmap = pixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            icon.addPixmap(pixmap);
        }

        return icon;
    }

    // Generated icon sizes should match ItemLibraryResourceView icon sizes
    QList<QSize> iconSizes  = {{192, 192}, {96, 96}, {48, 48}, {32, 32}};

};

CustomFileSystemModel::CustomFileSystemModel(QObject *parent) : QAbstractListModel(parent)
  , m_fileSystemModel(new QFileSystemModel(this))
  , m_fileSystemWatcher(new Utils::FileSystemWatcher(this))
{
    m_fileSystemModel->setIconProvider(new ItemLibraryFileIconProvider());

    connect(m_fileSystemWatcher, &Utils::FileSystemWatcher::directoryChanged, [this] {
        updatePath(m_fileSystemModel->rootPath());
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

void CustomFileSystemModel::appendIfNotFiltered(const QString &file)
{
    if (filterMetaIcons(file))
        m_files.append(file);
}

QModelIndex CustomFileSystemModel::updatePath(const QString &newPath)
{
    beginResetModel();
    m_fileSystemModel->setRootPath(newPath);

    m_fileSystemWatcher->removeDirectories(m_fileSystemWatcher->directories());

    m_fileSystemWatcher->addDirectory(newPath, Utils::FileSystemWatcher::WatchAllChanges);

    QStringList nameFilterList;

    const QString searchFilter = m_searchFilter;

    if (searchFilter.contains(QLatin1Char('.'))) {
        nameFilterList.append(QString(QStringLiteral("*%1*")).arg(searchFilter));
    } else {
        const QString filterTemplate("*%1*.%2");
        for (const QString &ext : supportedImageSuffixes())
            nameFilterList.append(filterTemplate.arg(searchFilter, ext));
        for (const QString &ext : supportedShaderSuffixes())
            nameFilterList.append(filterTemplate.arg(searchFilter, ext));
        for (const QString &ext : supportedFontSuffixes())
            nameFilterList.append(filterTemplate.arg(searchFilter, ext));
        for (const QString &ext : supportedAudioSuffixes())
            nameFilterList.append(filterTemplate.arg(searchFilter, ext));
    }

    m_files.clear();

    QDirIterator fileIterator(newPath, nameFilterList, QDir::Files, QDirIterator::Subdirectories);

    while (fileIterator.hasNext())
        appendIfNotFiltered(fileIterator.next());

    QDirIterator dirIterator(newPath, {}, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIterator.hasNext())
        m_fileSystemWatcher->addDirectory(dirIterator.next(), Utils::FileSystemWatcher::WatchAllChanges);

    endResetModel();
    return QAbstractListModel::index(0, 0);
}

QModelIndex CustomFileSystemModel::fileSystemModelIndex(const QModelIndex &index) const
{
    const int row = index.row();
    return m_fileSystemModel->index(m_files.at(row));
}

} //QmlDesigner
