/**************************************************************************
**
** Copyright (C) 2015 Denis Mingulov.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "imageviewerfile.h"
#include "imageviewer.h"
#include "imageviewerconstants.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QGraphicsPixmapItem>
#ifndef QT_NO_SVG
#include <QGraphicsSvgItem>
#endif
#include <QImageReader>
#include <QMovie>
#include <QPainter>
#include <QPixmap>

namespace ImageViewer {
namespace Internal {

class MovieItem : public QObject, public QGraphicsPixmapItem
{
public:
    MovieItem(QMovie *movie)
        : m_movie(movie)
    {
        setPixmap(m_movie->currentPixmap());
        connect(m_movie, &QMovie::updated, this, [this](const QRectF &rect) {
            update(rect);
        });
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
    {
        const bool smoothTransform = painter->worldTransform().m11() < 1;
        painter->setRenderHint(QPainter::SmoothPixmapTransform, smoothTransform);
        painter->drawPixmap(offset(), m_movie->currentPixmap());
    }

private:
    QMovie *m_movie;
};

ImageViewerFile::ImageViewerFile()
{
    setId(Constants::IMAGEVIEWER_ID);
    connect(this, &ImageViewerFile::mimeTypeChanged, this, &ImageViewerFile::changed);
}

ImageViewerFile::~ImageViewerFile()
{
    cleanUp();
}

Core::IDocument::OpenResult ImageViewerFile::open(QString *errorString, const QString &fileName,
                                                  const QString &realFileName)
{
    QTC_CHECK(fileName == realFileName); // does not support auto save
    OpenResult success = openImpl(errorString, fileName);
    emit openFinished(success == OpenResult::Success);
    return success;
}

Core::IDocument::OpenResult ImageViewerFile::openImpl(QString *errorString, const QString &fileName)
{
    cleanUp();

    if (!QFileInfo(fileName).isReadable())
        return OpenResult::ReadError;

    QByteArray format = QImageReader::imageFormat(fileName);
    // if it is impossible to recognize a file format - file will not be open correctly
    if (format.isEmpty()) {
        if (errorString)
            *errorString = tr("Image format not supported.");
        return OpenResult::CannotHandle;
    }

#ifndef QT_NO_SVG
    if (format.startsWith("svg")) {
        m_tempSvgItem = new QGraphicsSvgItem(fileName);
        QRectF bound = m_tempSvgItem->boundingRect();
        if (bound.width() == 0 && bound.height() == 0) {
            delete m_tempSvgItem;
            m_tempSvgItem = 0;
            if (errorString)
                *errorString = tr("Failed to read SVG image.");
            return OpenResult::CannotHandle;
        }
        m_type = TypeSvg;
        emit imageSizeChanged(QSize());
    } else
#endif
    if (QMovie::supportedFormats().contains(format)) {
        m_type = TypeMovie;
        m_movie = new QMovie(fileName, QByteArray(), this);
        m_movie->setCacheMode(QMovie::CacheAll);
        connect(m_movie, &QMovie::finished, m_movie, &QMovie::start);
        connect(m_movie, &QMovie::resized, this, &ImageViewerFile::imageSizeChanged);
        m_movie->start();
        m_isPaused = false; // force update
        setPaused(true);
    } else {
        m_pixmap = new QPixmap(fileName);
        if (m_pixmap->isNull()) {
            if (errorString)
                *errorString = tr("Failed to read image.");
            delete m_pixmap;
            m_pixmap = 0;
            return OpenResult::CannotHandle;
        }
        m_type = TypePixmap;
        emit imageSizeChanged(m_pixmap->size());
    }

    setFilePath(Utils::FileName::fromString(fileName));
    Utils::MimeDatabase mdb;
    setMimeType(mdb.mimeTypeForFile(fileName).name());
    return OpenResult::Success;
}

Core::IDocument::ReloadBehavior ImageViewerFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    if (type == TypeRemoved || type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents && state == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

bool ImageViewerFile::reload(QString *errorString,
                             Core::IDocument::ReloadFlag flag,
                             Core::IDocument::ChangeType type)
{
    if (flag == FlagIgnore)
        return true;
    if (type == TypePermissions) {
        emit changed();
        return true;
    }
    emit aboutToReload();
    bool success = (openImpl(errorString, filePath().toString()) == OpenResult::Success);
    emit reloadFinished(success);
    return success;
}

bool ImageViewerFile::isPaused() const
{
    return m_isPaused;
}

void ImageViewerFile::setPaused(bool paused)
{
    if (!m_movie || m_isPaused == paused)
        return;
    m_isPaused = paused;
    m_movie->setPaused(paused);
    emit isPausedChanged(m_isPaused);
}

QGraphicsItem *ImageViewerFile::createGraphicsItem() const
{
    QGraphicsItem *val = 0;
    switch (m_type) {
    case TypeInvalid:
        break;
    case TypeSvg:
#ifndef QT_NO_SVG
        if (m_tempSvgItem) {
            val = m_tempSvgItem;
            m_tempSvgItem = 0;
        } else {
            val = new QGraphicsSvgItem(filePath().toString());
        }
#endif
        break;
    case TypeMovie:
        val = new MovieItem(m_movie);
        break;
    case TypePixmap: {
        auto pixmapItem = new QGraphicsPixmapItem(*m_pixmap);
        pixmapItem->setTransformationMode(Qt::SmoothTransformation);
        val = pixmapItem;
        break;
    }
    default:
        break;
    }
    return val;
}

ImageViewerFile::ImageType ImageViewerFile::type() const
{
    return m_type;
}

void ImageViewerFile::updateVisibility()
{
    if (!m_movie || m_isPaused)
        return;
    bool visible = false;
    foreach (Core::IEditor *editor, Core::DocumentModel::editorsForDocument(this)) {
        if (editor->widget()->isVisible()) {
            visible = true;
            break;
        }
    }
    m_movie->setPaused(!visible);
}

void ImageViewerFile::cleanUp()
{
    delete m_pixmap;
    m_pixmap = 0;
    delete m_movie;
    m_movie = 0;
#ifndef QT_NO_SVG
    delete m_tempSvgItem;
    m_tempSvgItem = 0;
#endif
    m_type = TypeInvalid;
}

bool ImageViewerFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave)
    return false;
}

bool ImageViewerFile::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents);
    return false;
}

QString ImageViewerFile::defaultPath() const
{
    return QString();
}

QString ImageViewerFile::suggestedFileName() const
{
    return QString();
}

bool ImageViewerFile::isModified() const
{
    return false;
}

bool ImageViewerFile::isSaveAsAllowed() const
{
    return false;
}

} // namespace Internal
} // namespace ImageViewer
