/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Custom Merge QtDesignStudio plugin.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/

#include "qrcodeimageprovider.h"
#include "qrcodegen.h"
#include <QPainter>
#include <QSvgRenderer>

QrCodeImageProvider::QrCodeImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}


static QString qrToSvgString(const qrcodegen::QrCode &qr, int border) {
    if (border < 0)
        throw std::domain_error("Border must be non-negative");
    if (border > INT_MAX / 2 || border * 2 > INT_MAX - qr.getSize())
        throw std::overflow_error("Border too large");

    QString svgString;
    QTextStream stream(&svgString);

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    stream << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    stream << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
    stream << (qr.getSize() + border * 2) << " " << (qr.getSize() + border * 2) << "\" stroke=\"none\">\n";
    stream << "\t<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>\n";
    stream << "\t<path d=\"";
    for (int y = 0; y < qr.getSize(); y++) {
        for (int x = 0; x < qr.getSize(); x++) {
            if (qr.getModule(x, y)) {
                if (x != 0 || y != 0)
                    stream << " ";
                stream << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
            }
        }
    }
    stream << "\" fill=\"#000000\"/>\n";
    stream << "</svg>\n";

    return svgString;

}

QPixmap QrCodeImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    int width = 1000;
    int height = 1000;

    const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(id.toLatin1(), qrcodegen::QrCode::Ecc::LOW);

    QString svgString = qrToSvgString(qr, 3);
    QSvgRenderer svgRenderer(svgString.toLatin1());


    if (size)
        *size = QSize(width, height);

    QPixmap pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
                 requestedSize.height() > 0 ? requestedSize.height() : height);

    QPainter painter(&pixmap);

    svgRenderer.render(&painter);

    return pixmap;

}


