/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov.
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

#pragma once

#include <QGraphicsView>

QT_FORWARD_DECLARE_CLASS(QImage)

namespace ImageViewer {
namespace Internal {

class ImageViewerFile;

struct ExportData {
    QString fileName;
    QSize size;
};

class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    ImageView(ImageViewerFile *file);
    ~ImageView() override;

    void reset();
    void createScene();

    void exportImage();
    void exportMultiImages();
    void copyDataUrl();
    void setViewBackground(bool enable);
    void setViewOutline(bool enable);
    void zoomIn();
    void zoomOut();
    void resetToOriginalSize();
    void fitToScreen();

signals:
    void scaleFactorChanged(qreal factor);
    void imageSizeChanged(const QSize &size);

private:
    void emitScaleFactor();
    void doScale(qreal factor);
    QSize svgSize() const;
    QImage renderSvg(const QSize &imageSize) const;
    bool exportSvg(const ExportData &ed);

    void drawBackground(QPainter *p, const QRectF &rect) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    ImageViewerFile *m_file;
    QGraphicsItem *m_imageItem = nullptr;
    QGraphicsRectItem *m_backgroundItem = nullptr;
    QGraphicsRectItem *m_outlineItem = nullptr;
    bool m_showBackground = false;
    bool m_showOutline = true;
};

} // namespace Internal
} // namespace ImageViewer
