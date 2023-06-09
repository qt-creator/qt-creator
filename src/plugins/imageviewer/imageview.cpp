// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imageview.h"

#include "exportdialog.h"
#include "imageviewerfile.h"
#include "imageviewertr.h"
#include "multiexportdialog.h"

#include <coreplugin/messagemanager.h>

#include <utils/fileutils.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsRectItem>
#include <QGuiApplication>
#include <QImage>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QWheelEvent>

#include <qmath.h>

#ifndef QT_NO_SVG
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#endif

const char kSettingsGroup[] = "ImageViewer";
const char kSettingsBackground[] = "ShowBackground";
const char kSettingsOutline[] = "ShowOutline";
const char kSettingsFitToScreen[] = "FitToScreen";

using namespace Utils;

namespace ImageViewer {
namespace Constants {
    const qreal DEFAULT_SCALE_FACTOR = 1.2;
    const qreal zoomLevels[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 4.0, 8.0 };
}

namespace Internal {

static qreal nextLevel(qreal currentLevel)
{
    auto iter = std::find_if(std::begin(Constants::zoomLevels), std::end(Constants::zoomLevels), [&](qreal val) {
        return val > currentLevel;
    });
    if (iter != std::end(Constants::zoomLevels))
        return *iter;
    return currentLevel;
}

static qreal previousLevel(qreal currentLevel)
{
    auto iter = std::find_if(std::rbegin(Constants::zoomLevels), std::rend(Constants::zoomLevels), [&](qreal val) {
        return val < currentLevel;
    });
    if (iter != std::rend(Constants::zoomLevels))
        return *iter;
    return currentLevel;
}

ImageView::ImageView(ImageViewerFile *file)
    : m_file(file)
{
    setScene(new QGraphicsScene(this));
    setTransformationAnchor(AnchorUnderMouse);
    setDragMode(ScrollHandDrag);
    setInteractive(false);
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

ImageView::~ImageView() = default;

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
    m_backgroundItem->setVisible(m_settings.showBackground);
    m_backgroundItem->setZValue(-1);

    // outline
    m_outlineItem = new QGraphicsRectItem(m_imageItem->boundingRect());
    QPen outline(Qt::black, 1, Qt::DashLine);
    outline.setCosmetic(true);
    m_outlineItem->setPen(outline);
    m_outlineItem->setBrush(Qt::NoBrush);
    m_outlineItem->setVisible(m_settings.showOutline);
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
    p->setRenderHint(QPainter::SmoothPixmapTransform, false);
    p->drawTiledPixmap(viewport()->rect(), backgroundBrush().texture());
    p->restore();
}

QImage ImageView::renderSvg(const QSize &imageSize) const
{
    QImage image(imageSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter;
    painter.begin(&image);
#ifndef QT_NO_SVG
    auto svgItem = qgraphicsitem_cast<QGraphicsSvgItem *>(m_imageItem);
    QTC_ASSERT(svgItem, return image);
    svgItem->renderer()->render(&painter, QRectF(QPointF(), QSizeF(imageSize)));
#endif
    painter.end();
    return image;
}

bool ImageView::exportSvg(const ExportData &ed)
{
    const bool result = renderSvg(ed.size).save(ed.filePath.toFSPathString());
    if (result) {
        const QString message = Tr::tr("Exported \"%1\", %2x%3, %4 bytes")
            .arg(ed.filePath.toUserOutput())
            .arg(ed.size.width()).arg(ed.size.height())
            .arg(ed.filePath.fileSize());
        Core::MessageManager::writeDisrupting(message);
    } else {
        const QString message = Tr::tr("Could not write file \"%1\".").arg(ed.filePath.toUserOutput());
        QMessageBox::critical(this, Tr::tr("Export Image"), message);
    }
    return result;
}

#ifndef QT_NO_SVG
static FilePath suggestedExportFileName(const FilePath &fi)
{
    return fi.absolutePath().pathAppended(fi.baseName() + ".png");
}
#endif

QSize ImageView::svgSize() const
{
    QSize result;
#ifndef QT_NO_SVG
    if (const QGraphicsSvgItem *svgItem = qgraphicsitem_cast<QGraphicsSvgItem *>(m_imageItem))
        result = svgItem->boundingRect().size().toSize();
#endif // !QT_NO_SVG
    return result;
}

void ImageView::exportImage()
{
#ifndef QT_NO_SVG
    auto svgItem = qgraphicsitem_cast<QGraphicsSvgItem *>(m_imageItem);
    QTC_ASSERT(svgItem, return);

    const FilePath origPath = m_file->filePath();
    ExportDialog exportDialog(this);
    exportDialog.setWindowTitle(Tr::tr("Export %1").arg(origPath.fileName()));
    exportDialog.setExportSize(svgSize());
    exportDialog.setExportFileName(suggestedExportFileName(origPath));

    while (exportDialog.exec() == QDialog::Accepted && !exportSvg(exportDialog.exportData())) {}
#endif // !QT_NO_SVG
}

void ImageView::exportMultiImages()
{
#ifndef QT_NO_SVG
    QTC_ASSERT(qgraphicsitem_cast<QGraphicsSvgItem *>(m_imageItem), return);

    const FilePath origPath = m_file->filePath();
    const QSize size = svgSize();
    const QString title = Tr::tr("Export a Series of Images from %1 (%2x%3)")
          .arg(origPath.fileName()).arg(size.width()).arg(size.height());
    MultiExportDialog multiExportDialog;
    multiExportDialog.setWindowTitle(title);
    multiExportDialog.setExportFileName(suggestedExportFileName(origPath));
    multiExportDialog.setSvgSize(size);
    multiExportDialog.suggestSizes();

    while (multiExportDialog.exec() == QDialog::Accepted) {
        const auto exportData = multiExportDialog.exportData();
        bool ok = true;
        for (const auto &data : exportData) {
            if (!exportSvg(data)) {
                ok = false;
                break;
            }
        }
        if (ok)
            break;
    }
#endif // !QT_NO_SVG
}

void ImageView::copyDataUrl()
{
    Utils::MimeType mimeType = Utils::mimeTypeForFile(m_file->filePath());
    QByteArray data = m_file->filePath().fileContents().value_or(QByteArray());
    const auto url = QStringLiteral("data:%1;base64,%2")
            .arg(mimeType.name())
            .arg(QString::fromLatin1(data.toBase64()));
    QGuiApplication::clipboard()->setText(url);
}

void ImageView::setViewBackground(bool enable)
{
    m_settings.showBackground = enable;
    if (m_backgroundItem)
        m_backgroundItem->setVisible(enable);
}

void ImageView::setViewOutline(bool enable)
{
    m_settings.showOutline = enable;
    if (m_outlineItem)
        m_outlineItem->setVisible(enable);
}

void ImageView::doScale(qreal factor)
{
    scale(factor, factor);
    emitScaleFactor();
    if (auto pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(m_imageItem))
        pixmapItem->setTransformationMode(
                    transform().m11() < 1 ? Qt::SmoothTransformation : Qt::FastTransformation);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    setFitToScreen(false);
    qreal factor = qPow(Constants::DEFAULT_SCALE_FACTOR, event->angleDelta().y() / 240.0);
    // cap to 0.001 - 1000
    qreal actualFactor = qBound(0.001, factor, 1000.0);
    doScale(actualFactor);
    event->accept();
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (m_settings.fitToScreen)
        doFitToScreen();
}

void ImageView::zoomIn()
{
    setFitToScreen(false);
    qreal nextZoomLevel = nextLevel(transform().m11());
    resetTransform();
    doScale(nextZoomLevel);
}

void ImageView::zoomOut()
{
    setFitToScreen(false);
    qreal previousZoomLevel = previousLevel(transform().m11());
    resetTransform();
    doScale(previousZoomLevel);
}

void ImageView::resetToOriginalSize()
{
    setFitToScreen(false);
    resetTransform();
    emitScaleFactor();
}

void ImageView::setFitToScreen(bool fit)
{
    if (fit == m_settings.fitToScreen)
        return;
    m_settings.fitToScreen = fit;
    if (m_settings.fitToScreen)
        doFitToScreen();
    emit fitToScreenChanged(m_settings.fitToScreen);
}

void ImageView::readSettings(Utils::QtcSettings *settings)
{
    const Settings def;
    settings->beginGroup(kSettingsGroup);
    m_settings.showBackground = settings->value(kSettingsBackground, def.showBackground).toBool();
    m_settings.showOutline = settings->value(kSettingsOutline, def.showOutline).toBool();
    m_settings.fitToScreen = settings->value(kSettingsFitToScreen, def.fitToScreen).toBool();
    settings->endGroup();
}

void ImageView::writeSettings(Utils::QtcSettings *settings) const
{
    const Settings def;
    settings->beginGroup(kSettingsGroup);
    settings->setValueWithDefault(kSettingsBackground,
                                  m_settings.showBackground,
                                  def.showBackground);
    settings->setValueWithDefault(kSettingsOutline, m_settings.showOutline, def.showOutline);
    settings->setValueWithDefault(kSettingsFitToScreen, m_settings.fitToScreen, def.fitToScreen);
    settings->endGroup();
}

ImageView::Settings ImageView::settings() const
{
    return m_settings;
}

void ImageView::doFitToScreen()
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
