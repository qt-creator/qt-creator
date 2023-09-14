// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imagecachefontcollector.h"

#include <QtGui/qrawfont.h>
#include <QtGui/qpainter.h>
#include <QtGui/qimage.h>
#include <QtGui/qfontdatabase.h>
#include <QtGui/qfontmetrics.h>
#include <QtGui/qicon.h>
#include <QtCore/qfileinfo.h>

namespace QmlDesigner {

ImageCacheFontCollector::ImageCacheFontCollector() = default;

ImageCacheFontCollector::~ImageCacheFontCollector() = default;

namespace {

QByteArray fileToByteArray(const QString &filename)
{
    QFile file(filename);
    QFileInfo fileInfo(file);

    if (fileInfo.exists() && file.open(QFile::ReadOnly))
        return file.readAll();

    return {};
}

// Returns font id or -1 in case of failure
static int resolveFont(const QString &fontFile, QFont &outFont)
{
    int fontId = -1;
    QByteArray fontData(fileToByteArray(fontFile));
    if (!fontData.isEmpty()) {
        fontId = QFontDatabase::addApplicationFontFromData(fontData);
        if (fontId != -1) {
            QRawFont rawFont(fontData, 10.); // Pixel size is irrelevant, we only need style/weight
            const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                QString fontFamily = families.first();
                outFont.setFamily(fontFamily);
                outFont.setStyle(rawFont.style());
                outFont.setStyleName(rawFont.styleName());
                outFont.setWeight(QFont::Weight(rawFont.weight()));
            }
        }
    }
    return fontId;
}

static QImage createFontImage(const QString &text, const QColor &textColor, const QFont &font,
                              const QSize &size)
{
    QRect rect({0, 0}, size);
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    int pixelSize(200);
    int flags = Qt::AlignCenter;
    QFont renderFont = font;
    while (pixelSize >= 2) {
        renderFont.setPixelSize(pixelSize);
        QFontMetrics fm(renderFont, &image);
        QRect bounds = fm.boundingRect(rect, flags, text);
        if (bounds.width() < rect.width() && bounds.height() < rect.height()) {
            break;
        } else {
            int newPixelSize = pixelSize - 1;
            if (bounds.width() >= rect.width())
                newPixelSize = int(qreal(pixelSize) * qreal(rect.width()) / qreal(bounds.width()));
            else if (bounds.height() >= rect.height())
                newPixelSize = int(qreal(pixelSize) * qreal(rect.height()) / qreal(bounds.height()));
            if (newPixelSize < pixelSize)
                pixelSize = newPixelSize;
            else
                --pixelSize;
        }
    }

    QPainter painter(&image);
    painter.setPen(textColor);
    painter.setFont(renderFont);
    painter.drawText(rect, flags, text);

    return image;
}

} // namespace

void ImageCacheFontCollector::start(Utils::SmallStringView name,
                                    Utils::SmallStringView,
                                    const ImageCache::AuxiliaryData &auxiliaryDataValue,
                                    CaptureCallback captureCallback,
                                    AbortCallback abortCallback)
{
    QFont font;
    if (resolveFont(QString(name), font) >= 0) {
        if (auto auxiliaryData = std::get_if<ImageCache::FontCollectorSizeAuxiliaryData>(
                &auxiliaryDataValue)) {
            QColor textColor = auxiliaryData->colorName;
            QSize size = auxiliaryData->size;
            QString text = font.family() + "\n" + auxiliaryData->text;

            QImage image = createFontImage(text, textColor, font, size);

            if (!image.isNull()) {
                captureCallback(std::move(image), {}, {});
                return;
            }
        }
    }
    abortCallback(ImageCache::AbortReason::Failed);
}

ImageCacheCollectorInterface::ImageTuple ImageCacheFontCollector::createImage(
    Utils::SmallStringView name,
    Utils::SmallStringView,
    const ImageCache::AuxiliaryData &auxiliaryDataValue)
{
    QFont font;
    if (resolveFont(QString(name), font) >= 0) {
        if (auto auxiliaryData = std::get_if<ImageCache::FontCollectorSizeAuxiliaryData>(
                &auxiliaryDataValue)) {
            QColor textColor = auxiliaryData->colorName;
            QSize size = auxiliaryData->size;
            QString text = font.family() + "\n\n" + auxiliaryData->text;

            QImage image = createFontImage(text, textColor, font, size);

            if (!image.isNull())
                return {image, {}, {}};
        }
    }

    return {};
}

QIcon ImageCacheFontCollector::createIcon(Utils::SmallStringView name,
                                          Utils::SmallStringView,
                                          const ImageCache::AuxiliaryData &auxiliaryDataValue)
{
    QIcon icon;

    QFont font;
    if (resolveFont(QString(name), font) >= 0) {
        if (auto auxiliaryData = std::get_if<ImageCache::FontCollectorSizesAuxiliaryData>(
                &auxiliaryDataValue)) {
            QColor textColor = auxiliaryData->colorName;
            const auto sizes = auxiliaryData->sizes;
            QString text = auxiliaryData->text;

            for (QSize size : sizes) {
                QImage image = createFontImage(text, textColor, font, size);
                if (!image.isNull())
                    icon.addPixmap(QPixmap::fromImage(image));
            }
        }
    }

    return icon;
}

} // namespace QmlDesigner
