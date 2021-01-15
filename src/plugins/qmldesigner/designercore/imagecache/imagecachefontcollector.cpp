/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "imagecachefontcollector.h"

#ifndef QMLDESIGNER_TEST // Tests don't care about UI, and can't have theme dependency here
#include <theme.h>
#endif

#include <QtGui/qrawfont.h>
#include <QtGui/qpainter.h>
#include <QtGui/qimage.h>
#include <QtGui/qfontdatabase.h>
#include <QtGui/qfontmetrics.h>
#include <QtCore/qfileinfo.h>

namespace QmlDesigner {

ImageCacheFontCollector::ImageCacheFontCollector() = default;

ImageCacheFontCollector::~ImageCacheFontCollector() = default;

QByteArray fileToByteArray(QString const &filename)
{
    QFile file(filename);
    QFileInfo fileInfo(file);

    if (fileInfo.exists() && file.open(QFile::ReadOnly))
        return file.readAll();

    return {};
}

void ImageCacheFontCollector::start(Utils::SmallStringView name,
                                    Utils::SmallStringView state,
                                    CaptureCallback captureCallback,
                                    AbortCallback abortCallback)
{
    // State contains size, text color, and sample text
    QStringList hints = QString(state).split('@');
    int dim(300);
    if (hints.size() >= 1) {
        bool ok = false;
        int newDim = QString(hints[0]).toInt(&ok);
        if (ok)
            dim = newDim;
    }
#ifndef QMLDESIGNER_TEST
    QColor textColor(Theme::getColor(Theme::DStextColor));
#else
    QColor textColor;
#endif
    if (hints.size() >= 2)
        textColor.setNamedColor(hints[1]);
    QString text = hints.size() >= 3 ? hints[2] : "Abc";
    QSize size(dim, dim);
    QRect rect({0, 0}, size);

    QByteArray fontData(fileToByteArray(QString(name)));
    if (!fontData.isEmpty()) {
        int fontId = QFontDatabase::addApplicationFontFromData(fontData);
        if (fontId != -1) {
            QRawFont rawFont(fontData, 10.); // Pixel size is irrelevant, we only need style/weight
            const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                QString fontFamily = families.first();
                if (text.contains("%1"))
                    text = text.arg(fontFamily);
                QFont font(fontFamily);
                font.setStyle(rawFont.style());
                font.setStyleName(rawFont.styleName());
                font.setWeight(rawFont.weight());
                QImage image(size, QImage::Format_ARGB32);
                image.fill(Qt::transparent);
                int pixelSize(200);
                int flags = Qt::AlignCenter;
                while (pixelSize >= 2) {
                    font.setPixelSize(pixelSize);
                    QFontMetrics fm(font, &image);
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
                painter.setFont(font);
                painter.drawText(rect, flags, text);

                captureCallback(std::move(image));
                return;
            }
            QFontDatabase::removeApplicationFont(fontId);
        }
    }
    abortCallback();
}

} // namespace QmlDesigner
