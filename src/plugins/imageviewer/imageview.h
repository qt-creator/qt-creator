// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsView>

QT_FORWARD_DECLARE_CLASS(QImage)

namespace ImageViewer::Internal {

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

} // ImageViewer::Internal
