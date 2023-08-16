// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QGraphicsView>

QT_BEGIN_NAMESPACE
class QImage;
QT_END_NAMESPACE

namespace Utils {
class QtcSettings;
}

namespace ImageViewer::Internal {

class ImageViewerFile;

struct ExportData {
    Utils::FilePath filePath;
    QSize size;
};

class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    struct Settings
    {
        bool showBackground = false;
        bool showOutline = true;
        bool fitToScreen = false;
    };

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
    void setFitToScreen(bool fit);

    void readSettings(Utils::QtcSettings *settings);
    void writeSettings(Utils::QtcSettings *settings) const;
    Settings settings() const;

signals:
    void scaleFactorChanged(qreal factor);
    void imageSizeChanged(const QSize &size);
    void fitToScreenChanged(bool fit);

private:
    void doFitToScreen();
    void emitScaleFactor();
    void doScale(qreal factor);
    QSize svgSize() const;
    QImage renderSvg(const QSize &imageSize) const;
    bool exportSvg(const ExportData &ed);

    void drawBackground(QPainter *p, const QRectF &rect) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    ImageViewerFile *m_file;
    QGraphicsItem *m_imageItem = nullptr;
    QGraphicsRectItem *m_backgroundItem = nullptr;
    QGraphicsRectItem *m_outlineItem = nullptr;
    Settings m_settings;
};

} // ImageViewer::Internal
