/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov.
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Denis Mingulov.
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "imageview.h"

#include "exportdialog.h"
#include "imageviewerfile.h"

#include <coreplugin/messagemanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QGraphicsRectItem>

#include <QWheelEvent>
#include <QMouseEvent>
#include <QImage>
#include <QPainter>
#include <QPixmap>

#include <QDir>
#include <QFileInfo>

#include <qmath.h>

#ifndef QT_NO_SVG
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#endif

namespace ImageViewer {
namespace Constants {
    const qreal DEFAULT_SCALE_FACTOR = 1.2;
}

namespace Internal {

ImageView::ImageView(ImageViewerFile *file)
    : m_file(file)
{
    setScene(new QGraphicsScene(this));
    setTransformationAnchor(AnchorUnderMouse);
    setDragMode(ScrollHandDrag);
    setViewportUpdateMode(FullViewportUpdate);
    setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::SmoothPixmapTransform);

    // Prepare background check-board pattern
    QPixmap tilePixmap(64, 64);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, 0x20, 0x20, color);
    tilePainter.fillRect(0x20, 0x20, 0x20, 0x20, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);
}

ImageView::~ImageView()
{
}

void ImageView::reset()
{
    scene()->clear();
    resetTransform();
}

void ImageView::createScene()
{
    m_imageItem = m_file->createGraphicsItem();
    if (!m_imageItem) // failed to load
        return;
    m_imageItem->setCacheMode(QGraphicsItem::NoCache);
    m_imageItem->setZValue(0);

    // background item
    m_backgroundItem = new QGraphicsRectItem(m_imageItem->boundingRect());
    m_backgroundItem->setBrush(Qt::white);
    m_backgroundItem->setPen(Qt::NoPen);
    m_backgroundItem->setVisible(m_showBackground);
    m_backgroundItem->setZValue(-1);

    // outline
    m_outlineItem = new QGraphicsRectItem(m_imageItem->boundingRect());
    QPen outline(Qt::black, 1, Qt::DashLine);
    outline.setCosmetic(true);
    m_outlineItem->setPen(outline);
    m_outlineItem->setBrush(Qt::NoBrush);
    m_outlineItem->setVisible(m_showOutline);
    m_outlineItem->setZValue(1);

    QGraphicsScene *s = scene();
    s->addItem(m_backgroundItem);
    s->addItem(m_imageItem);
    s->addItem(m_outlineItem);

    emitScaleFactor();
}

void ImageView::drawBackground(QPainter *p, const QRectF &)
{
    p->save();
    p->resetTransform();
    p->drawTiledPixmap(viewport()->rect(), backgroundBrush().texture());
    p->restore();
}

void ImageView::exportImage()
{
#ifndef QT_NO_SVG
    QGraphicsSvgItem *svgItem = qgraphicsitem_cast<QGraphicsSvgItem *>(m_imageItem);
    QTC_ASSERT(svgItem, return);

    const QFileInfo origFi = m_file->filePath().toFileInfo();
    const QString suggestedFileName = origFi.absolutePath() + QLatin1Char('/')
        + origFi.baseName() + QStringLiteral(".png");

    ExportDialog exportDialog(this);
    exportDialog.setWindowTitle(tr("Export %1").arg(origFi.fileName()));
    exportDialog.setExportSize(svgItem->boundingRect().size().toSize());
    exportDialog.setExportFileName(suggestedFileName);

    while (true) {
        if (exportDialog.exec() != QDialog::Accepted)
            break;

        const QSize imageSize = exportDialog.exportSize();
        QImage image(imageSize, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter;
        painter.begin(&image);
        svgItem->renderer()->render(&painter, QRectF(QPointF(), QSizeF(imageSize)));
        painter.end();

        const QString fileName = exportDialog.exportFileName();
        if (image.save(fileName)) {
            const QString message = tr("Exported \"%1\", %2x%3, %4 bytes")
                .arg(QDir::toNativeSeparators(fileName)).arg(imageSize.width()).arg(imageSize.height())
                .arg(QFileInfo(fileName).size());
            Core::MessageManager::write(message);
            break;
        } else {
            QMessageBox::critical(this, tr("Export Image"),
                                  tr("Could not write file \"%1\".").arg(QDir::toNativeSeparators(fileName)));
        }
    }
#endif // !QT_NO_SVG
}

void ImageView::setViewBackground(bool enable)
{
    m_showBackground = enable;
    if (m_backgroundItem)
        m_backgroundItem->setVisible(enable);
}

void ImageView::setViewOutline(bool enable)
{
    m_showOutline = enable;
    if (m_outlineItem)
        m_outlineItem->setVisible(enable);
}

void ImageView::doScale(qreal factor)
{
    qreal currentScale = transform().m11();
    qreal newScale = currentScale * factor;
    qreal actualFactor = factor;
    // cap to 0.001 - 1000
    if (newScale > 1000)
        actualFactor = 1000./currentScale;
    else if (newScale < 0.001)
        actualFactor = 0.001/currentScale;

    scale(actualFactor, actualFactor);
    emitScaleFactor();
    if (QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(m_imageItem))
        pixmapItem->setTransformationMode(
                    transform().m11() < 1 ? Qt::SmoothTransformation : Qt::FastTransformation);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    qreal factor = qPow(Constants::DEFAULT_SCALE_FACTOR, event->delta() / 240.0);
    doScale(factor);
    event->accept();
}

void ImageView::zoomIn()
{
    doScale(Constants::DEFAULT_SCALE_FACTOR);
}

void ImageView::zoomOut()
{
    doScale(1. / Constants::DEFAULT_SCALE_FACTOR);
}

void ImageView::resetToOriginalSize()
{
    resetTransform();
    emitScaleFactor();
}

void ImageView::fitToScreen()
{
    fitInView(m_imageItem, Qt::KeepAspectRatio);
    emitScaleFactor();
}

void ImageView::emitScaleFactor()
{
    // get scale factor directly from the transform matrix
    qreal factor = transform().m11();
    emit scaleFactorChanged(factor);
}

void ImageView::showEvent(QShowEvent *)
{
    m_file->updateVisibility();
}

void ImageView::hideEvent(QHideEvent *)
{
    m_file->updateVisibility();
}

} // namespace Internal
} // namespace ImageView
